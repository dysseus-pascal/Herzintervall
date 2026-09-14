#include "night.h"
#include "hrv.h"

#define COOKIE_NIGHT 1

// Wakeups muessen etwas in der Zukunft liegen; und wenn wir gerade selbst von
// einem geweckt wurden, darf der naechste nicht auf dieselbe Minute fallen.
#define LEAD_S 120

static time_t s_next;

// Naechster Zeitpunkt, an dem es HZ_NIGHT_HOUR Uhr ist - heute, wenn die
// Stunde noch kommt, sonst morgen.
//
// Ohne mktime gerechnet, wie in den Schwesterapps: Mitternacht ist jetzt minus
// der Sekunden seit Mitternacht. Am Tag einer Zeitumstellung kann das um eine
// Stunde danebenliegen. Das ist hier ohne Belang - eine Messung um vier oder
// sechs statt fuenf ist immer noch eine Messung im Schlaf, und beim naechsten
// Start stellt sich der Wecker ohnehin neu.
static time_t prv_next_time(time_t now) {
#ifdef HZ_TEST_NIGHT
  // Pruefbau: in einer Minute statt um fuenf. Nur so laesst sich im Emulator
  // nachsehen, ob der Wecker die App wirklich oeffnet, die Messung anlaeuft
  // und die App sich danach wieder schliesst.
  return now + 60;
#else
  struct tm *lt = localtime(&now);
  const time_t midnight = now - (lt->tm_hour * 3600 + lt->tm_min * 60 + lt->tm_sec);
  time_t at = midnight + (time_t)HZ_NIGHT_HOUR * 3600;
  if (at <= now + LEAD_S) at += 86400;
  return at;
#endif
}

void night_schedule(void) {
  const time_t now = time(NULL);
  const time_t at = prv_next_time(now);

  // Erst abraeumen: sonst sammeln sich ueber die Tage Wecker an, und Pebble
  // erlaubt hoechstens acht je App.
  wakeup_cancel_all();
  s_next = 0;

  // notify_if_missed = true: war die Uhr um fuenf aus, wird die Messung beim
  // Einschalten nachgeholt. Lieber eine verspaetete als gar keine.
  const WakeupId id = wakeup_schedule(at, COOKIE_NIGHT, true);
  if (id < 0) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Wecker abgelehnt (%d)", (int)id);
    return;
  }
  s_next = at;
  APP_LOG(APP_LOG_LEVEL_INFO, "Nachtmessung in %d h %d min",
          (int)((at - now) / 3600), (int)(((at - now) % 3600) / 60));
}

time_t night_next(void) {
  return s_next;
}

bool night_launched_us(void) {
  if (launch_reason() != APP_LAUNCH_WAKEUP) return false;
  WakeupId id;
  int32_t cookie;
  if (!wakeup_get_launch_event(&id, &cookie)) return false;
  return cookie == COOKIE_NIGHT;
}
