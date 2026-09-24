#include <pebble.h>
#include "hrv.h"
#include "hrv_selftest.h"
#include "main_window.h"
#include "night.h"
#include "phone.h"
#include "strings.h"

// App-Glance im Starter: der letzte RMSSD, damit man ihn sieht, ohne die App
// zu oeffnen. Wird beim Beenden gesetzt.
static void prv_glance_reload(AppGlanceReloadSession *session, size_t limit, void *context) {
  if (limit < 1) return;
  char text[48];
  const uint16_t last = hrv_last_rmssd();
  if (last > 0) {
    snprintf(text, sizeof(text), S(STR_GLANCE_FMT), (int)last);
  } else {
    snprintf(text, sizeof(text), "%s", S(STR_GLANCE_NONE));
  }
  const AppGlanceSlice slice = {
    .layout = { .icon = APP_GLANCE_SLICE_DEFAULT_ICON, .subtitle_template_string = text },
    .expiration_time = APP_GLANCE_SLICE_NO_EXPIRATION,
  };
  app_glance_add_slice(session, slice);
}

static void prv_phone_changed(void) {
  main_window_refresh();
  hrv_phone_settled();
}

static void prv_init(void) {
  // Sprache der Uhr uebernehmen, bevor das erste Fenster Texte holt
  strings_refresh();

#ifdef HZ_SELFTEST
  // Nur im Pruefbau: die Rechnung gegen vorab bestimmte Erwartungen stellen.
  // Steht bewusst VOR dem ersten Fenster, damit das Log vollstaendig ist,
  // auch wenn die App danach sofort beendet wird.
  hrv_selftest_run();
#endif

  phone_init();
  // Aendert sich der Zustand der Uebergabe, muss der Schirm nachziehen - und
  // eine Nachtmessung weiss dann, dass sie fertig ist.
  phone_set_observer(prv_phone_changed);
  hrv_init(main_window_refresh);
  main_window_push();

  // DIE NACHT MISST JETZT KIESELSPORT. Dessen Hintergrund-Worker kommt die
  // ganze Nacht an den Sensor, ohne dass der Schirm angeht - das konnte diese
  // App nie. Liefen beide, wuerde die Uhr um fuenf doppelt messen und
  // Kiesel-Helper zwei Naechte bekommen. Deshalb bei JEDEM Start alle eigenen
  // Wecker abraeumen und keine neuen stellen: ein Wecker aus einer frueheren
  // Fassung ueberlebt das Update, erst dieser Aufruf wird ihn los.
  wakeup_cancel_all();

  // Hat uns trotzdem noch ein alter Wecker geoeffnet, nicht messen, sondern
  // still wieder gehen - wer um fuenf schlaeft, will keinen Schirm sehen.
  if (night_launched_us()) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Alter Wecker - keine Nachtmessung mehr");
    window_stack_pop_all(false);
  }
}

static void prv_deinit(void) {
  // Der Sensor muss freigegeben werden, sonst laeuft er weiter und kostet
  // Batterie, auch wenn die App laengst zu ist.
  hrv_deinit();
  app_glance_reload(prv_glance_reload, NULL);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
  return 0;
}
