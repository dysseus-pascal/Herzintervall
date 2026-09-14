#include "hrv.h"
#include "phone.h"
#include "night.h"

#define PERSIST_RMSSD 1
#define PERSIST_TIME  2

static HrvPhase s_phase = HrvIdle;
static HrvStats s_stats;
static HrvChanged s_changed;
static AppTimer *s_tick;
static int s_left;
static int s_duration = HZ_MEASURE_S;
static bool s_night;
static bool s_quit_when_done;
static AppTimer *s_quit;

static void prv_quit_cb(void *data) {
  s_quit = NULL;
  APP_LOG(APP_LOG_LEVEL_INFO, "Nachtmessung beendet, App schliesst");
  window_stack_pop_all(false);
}
static uint16_t s_last_rmssd;
static time_t s_last_time;

// Im Selbsttest-Bau speist ein Taktgeber erfundene Intervalle ein, damit sich
// Ablauf und Anzeige im Emulator durchspielen lassen. Der Sensor ist dort
// nicht nachgebildet (CONFIG_HRM_HRV fehlt in qemu_emery), ohne das bliebe der
// Bildschirm bis zum Ende auf "Warte auf Schlaege" stehen.
#ifdef HZ_FAKE_BEATS
static AppTimer *s_fake;
static uint32_t s_fake_n;
#endif

static void prv_notify(void) {
  if (s_changed) s_changed();
}

static void prv_release_sensor(void) {
  // 0 gibt die Abtastperiode zurueck. Ohne das laeuft der Sensor weiter -
  // die Firmware haelt ihn an, solange IRGENDEINE App eine Periode haelt.
  health_service_set_hrv_sample_period(0);
}

static void prv_finish(void) {
  if (s_tick) {
    app_timer_cancel(s_tick);
    s_tick = NULL;
  }
#ifdef HZ_FAKE_BEATS
  if (s_fake) {
    app_timer_cancel(s_fake);
    s_fake = NULL;
  }
#endif
  prv_release_sensor();
  s_phase = HrvDone;

  const uint16_t rmssd = hrv_rmssd_ms(&s_stats);
  if (rmssd > 0) {
    s_last_rmssd = rmssd;
    s_last_time = time(NULL);
    persist_write_int(PERSIST_RMSSD, s_last_rmssd);
    persist_write_int(PERSIST_TIME, (int)s_last_time);
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "Messung fertig: %u ms aus %u Schlaegen (%u verworfen)",
          rmssd, (unsigned)s_stats.accepted, (unsigned)s_stats.rejected);
  // Ans Telefon reichen, damit die Companion-App es in Health Connect
  // schreiben kann. Nur ein brauchbares Ergebnis geht hinaus - eine Messung,
  // die zu wenig saubere Schlaege hatte, gehoert in keine Gesundheitsakte.
  phone_send_result(&s_stats, rmssd, s_last_time ? s_last_time : time(NULL));
  prv_notify();

  // Eine vom Wecker gestartete Nachtmessung schliesst die App wieder. NICHT
  // sofort: die Uebergabe ans Telefon laeuft noch, und ein Beenden mittendrin
  // risse sie ab. Auf die Bestaetigung wird gewartet - hrv_phone_settled()
  // macht dann Schluss.
  //
  // Der Zeitgeber hier ist nur der Rueckfall, falls weder Bestaetigung noch
  // Fehlschlag je eintreffen. Sonst bliebe die App die ganze Nacht offen.
  if (s_night && s_quit_when_done) {
    s_quit = app_timer_register(20000, prv_quit_cb, NULL);
  }
}

static void prv_tick_cb(void *data) {
  s_tick = NULL;
  if (s_phase != HrvMeasuring) return;
  if (--s_left <= 0) {
    s_left = 0;
    prv_finish();
    return;
  }
  s_tick = app_timer_register(1000, prv_tick_cb, NULL);
  prv_notify();
}

#ifdef HZ_FAKE_BEATS
// Erfundene Intervalle um 900 ms mit etwas Schwankung, dazu gelegentlich ein
// Ausreisser - damit auch der Filter im Emulator etwas zu tun bekommt.
static void prv_fake_cb(void *data) {
  s_fake = NULL;
  if (s_phase != HrvMeasuring) return;
  static const uint16_t pattern[] = { 900, 928, 884, 915, 1800, 906, 893, 921, 899, 912 };
  const uint16_t rr = pattern[s_fake_n % (sizeof(pattern) / sizeof(pattern[0]))];
  s_fake_n++;
  hrv_stats_add(&s_stats, rr);
  prv_notify();
  s_fake = app_timer_register(rr, prv_fake_cb, NULL);
}
#endif

static void prv_health_event(HealthEventType event, void *context) {
  if (event != HealthEventHRVUpdate || s_phase != HrvMeasuring) return;
  // Der Handler bekommt keinen Wert mitgeliefert - er muss geholt werden.
  // 0 heisst "nichts da"; das ist zugleich, was ausserhalb des Handgelenks
  // ankommt. Beides fuehrt hier zum selben: nichts zu zaehlen.
  const uint16_t ppi = health_service_peek_hrv_ppi_ms();
  if (ppi == 0) return;
  hrv_stats_add(&s_stats, ppi);
  prv_notify();
}

void hrv_init(HrvChanged on_change) {
  s_changed = on_change;
  hrv_stats_reset(&s_stats);
  s_phase = HrvIdle;
  if (persist_exists(PERSIST_RMSSD)) {
    const int v = persist_read_int(PERSIST_RMSSD);
    s_last_rmssd = (v > 0 && v < 0xFFFF) ? (uint16_t)v : 0;
  }
  if (persist_exists(PERSIST_TIME)) {
    s_last_time = (time_t)persist_read_int(PERSIST_TIME);
  }
  health_service_events_subscribe(prv_health_event, NULL);
}

void hrv_deinit(void) {
  if (s_tick) {
    app_timer_cancel(s_tick);
    s_tick = NULL;
  }
  if (s_quit) {
    app_timer_cancel(s_quit);
    s_quit = NULL;
  }
#ifdef HZ_FAKE_BEATS
  if (s_fake) {
    app_timer_cancel(s_fake);
    s_fake = NULL;
  }
#endif
  prv_release_sensor();
  health_service_events_unsubscribe();
}

static bool prv_start(int seconds, bool night) {
  hrv_stats_reset(&s_stats);
  s_duration = seconds;
  s_night = night;
  s_left = seconds;

  if (!health_service_set_hrv_sample_period(HZ_SAMPLE_S)) {
    // Zwei Ursachen, von aussen nicht unterscheidbar: kein HRV-faehiger Sensor
    // (alles ausser der Pebble Time 2, und auch der Emulator), oder die
    // Herzfrequenz ist in den Einstellungen der Uhr abgeschaltet.
    APP_LOG(APP_LOG_LEVEL_WARNING, "Uhr liefert kein HRV");
#ifndef HZ_FAKE_BEATS
    s_phase = HrvNoSensor;
    prv_notify();
    return false;
#endif
  }

  s_phase = HrvMeasuring;
  s_tick = app_timer_register(1000, prv_tick_cb, NULL);
#ifdef HZ_FAKE_BEATS
  s_fake_n = 0;
  s_fake = app_timer_register(900, prv_fake_cb, NULL);
#endif
  prv_notify();
  return true;
}

bool hrv_start(void) {
  return prv_start(HZ_MEASURE_S, false);
}

bool hrv_start_night(void) {
  // Nur beenden, wenn uns der Wecker geoeffnet hat. Wer die Nachtmessung von
  // Hand anstoesst, will danach nicht aus der App geworfen werden.
  s_quit_when_done = night_launched_us();
  return prv_start(HZ_NIGHT_S, true);
}

int hrv_duration(void) {
  return s_duration;
}

bool hrv_was_night(void) {
  return s_night;
}

void hrv_phone_settled(void) {
  if (!s_night || !s_quit_when_done || s_phase != HrvDone) return;
  const PhoneStatus st = phone_status();
  if (st != PhoneSent && st != PhoneFailed) return;   // noch unterwegs
  if (s_quit) {
    app_timer_cancel(s_quit);
    s_quit = NULL;
  }
  // Ein Wimpernschlag, damit die Zustandszeile noch kurz stehenbleibt - falls
  // doch jemand um fuenf hinsieht.
  s_quit = app_timer_register(800, prv_quit_cb, NULL);
}

void hrv_stop(void) {
  if (s_phase != HrvMeasuring) return;
  prv_finish();
}

HrvPhase hrv_phase(void) {
  return s_phase;
}

const HrvStats *hrv_result(void) {
  return &s_stats;
}

int hrv_seconds_left(void) {
  return s_left;
}

uint16_t hrv_last_rmssd(void) {
  return s_last_rmssd;
}

time_t hrv_last_time(void) {
  return s_last_time;
}
