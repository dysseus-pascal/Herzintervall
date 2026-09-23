#include "phone.h"
#include "nacht.h"
#include "hrv.h"   // HZ_MEASURE_S fuer die Deckungsrechnung

// Sechs Zahlenfelder zu je 11 Byte plus ein Byte fuer das Woerterbuch = 67.
// 128 laesst Luft fuer ein weiteres Feld, ohne dass jemand nachrechnen muss.
// Sechs Felder fuer die Messung, vier fuer die Nacht, je 11 Byte: 111.
#define OUTBOX_SIZE 160
#define INBOX_SIZE  64

static PhoneStatus s_status = PhoneNothing;
static int s_reason;
static void (*s_observer)(void);

static void prv_changed(void) {
  if (s_observer) s_observer();
}

static void prv_sent(DictionaryIterator *it, void *ctx) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Ergebnis ans Telefon uebergeben");
  s_status = PhoneSent;
  s_reason = 0;
  prv_changed();
}

static void prv_failed(DictionaryIterator *it, AppMessageResult reason, void *ctx) {
  // Haeufigster Fall: keine Companion-App installiert, oder das Telefon ist
  // gerade nicht da. Beides ist zu erwarten und kein Fehler der App.
  APP_LOG(APP_LOG_LEVEL_WARNING, "Telefon nicht erreicht (%d) - Wert bleibt auf der Uhr",
          (int)reason);
  s_status = PhoneFailed;
  s_reason = (int)reason;
  prv_changed();
}

PhoneStatus phone_status(void) {
  return s_status;
}

int phone_fail_reason(void) {
  return s_reason;
}

void phone_set_observer(void (*on_change)(void)) {
  s_observer = on_change;
}

void phone_init(void) {
  app_message_register_outbox_sent(prv_sent);
  app_message_register_outbox_failed(prv_failed);
  app_message_open(INBOX_SIZE, OUTBOX_SIZE);
}

void phone_send_result(const HrvStats *st, uint16_t rmssd_ms, time_t when) {
  // OHNE ERGEBNIS GEHT TROTZDEM ETWAS HINAUS: die Nacht. Eine Messung, die
  // an lockerem Band scheiterte, ist kein Grund, den Schlaf zu verschweigen.
  const bool mit_messung = st && rmssd_ms > 0;
  const Nacht nacht = nacht_lesen();
  if (!mit_messung && nacht.ende <= nacht.beginn) return;

  DictionaryIterator *out;
  if (app_message_outbox_begin(&out) != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Postausgang belegt - Ergebnis nicht geschickt");
    s_status = PhoneFailed;
    s_reason = (int)APP_MSG_BUSY;
    prv_changed();
    return;
  }
  s_status = PhoneSending;
  s_reason = 0;
  prv_changed();

  // Deckung in Prozent: wie viel der Messdauer die angenommenen Intervalle
  // zusammen ausfuellen. Dieselbe Rechnung wie auf dem Schirm, damit Uhr und
  // Telefon nicht zwei verschiedene Zahlen zeigen.
  // Mit der Dauer der TATSAECHLICHEN Messung rechnen, nicht mit der von
  // Hand gestarteten - sonst zeigte die Nachtmessung ein Drittel der Wahrheit.
  if (mit_messung) {
    uint32_t covered = (st->sum_rr / 10) / hrv_duration();
    if (covered > 100) covered = 100;

    dict_write_int32(out, MESSAGE_KEY_RMSSD,   (int32_t)rmssd_ms);
    dict_write_int32(out, MESSAGE_KEY_BPM,     (int32_t)hrv_mean_bpm(st));
    dict_write_int32(out, MESSAGE_KEY_BEATS,   (int32_t)st->accepted);
    dict_write_int32(out, MESSAGE_KEY_DROPPED, (int32_t)st->rejected);
    dict_write_int32(out, MESSAGE_KEY_COVERED, (int32_t)covered);
    // Zeitpunkt als Epoch-Sekunden. Die Companion-App braucht ihn, weil sie
    // den Messwert rueckwirkend in Health Connect eintraegt - nicht den
    // Augenblick, in dem sie ihn zufaellig erhaelt.
    dict_write_int32(out, MESSAGE_KEY_WHEN,    (int32_t)when);
  }

  // DIE NACHT FAEHRT MIT: die letzte abgeschlossene, mit ihren echten
  // Zeiten. Kiesel-Helper traegt sie ein - mit Riegel, also je Nacht einmal.
  if (nacht.ende > nacht.beginn) {
    dict_write_int32(out, MESSAGE_KEY_SLEEP_START,   (int32_t)nacht.beginn);
    dict_write_int32(out, MESSAGE_KEY_SLEEP_END,     (int32_t)nacht.ende);
    dict_write_int32(out, MESSAGE_KEY_SLEEP_RESTFUL, (int32_t)nacht.erholsam_s);
    if (nacht.ruhepuls > 0) {
      dict_write_int32(out, MESSAGE_KEY_RESTING_HR,  (int32_t)nacht.ruhepuls);
    }
  }

  app_message_outbox_send();
}
