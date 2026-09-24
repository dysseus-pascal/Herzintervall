#include "main_window.h"
#include "theme.h"
#include "hrv.h"
#include "phone.h"
#include "strings.h"

// Hauptscreen im Stil der Pebble-Timeline, wie Drinktervall und Flynformer:
// weisser Grund, schwarze Schrift, rechts die dunkle Seitenleiste mit Herz und
// Tastenhinweis. Links wie ein Timeline-Eintrag: kleine Uhrzeit, Kennzeile,
// grosse Zahl in LECO, Titel, Nebenzeile.
//
// Es gibt nur einen Bildschirm mit vier Zustaenden - mehr braucht es nicht:
//   bereit      letzter Wert (oder "noch keine Messung"), Mitteltaste startet
//   Messung     Restsekunden gross, Balken, gezaehlte Schlaege
//   Ergebnis    RMSSD gross, Puls und Anzahl darunter
//   kein HRV    was fehlt und was dagegen zu tun ist

#define BAR_SEGS 12
#define BAR_GAP  2
#define BAR_H    PBL_IF_ROUND_ELSE(12, (PBL_DISPLAY_WIDTH >= 180 ? 12 : 9))

static Window *s_window;
static Layer *s_canvas;

// Herz in der Seitenleiste: zwei Kreise und ein Dreieck, in Achteln der Breite
// gerechnet, damit es auf jedem Schirm gleich aussieht.
static void prv_draw_heart(GContext *ctx, GPoint c, int16_t w) {
  const int16_t r = w / 4;
  graphics_context_set_fill_color(ctx, HZ_COLOR_ON_SIDEBAR);
  graphics_fill_circle(ctx, GPoint(c.x - r, c.y - r / 2), r);
  graphics_fill_circle(ctx, GPoint(c.x + r, c.y - r / 2), r);
  GPoint pts[3] = {
    GPoint(c.x - 2 * r, c.y - r / 2),
    GPoint(c.x + 2 * r, c.y - r / 2),
    GPoint(c.x, c.y + w / 2),
  };
  const GPathInfo info = { .num_points = 3, .points = pts };
  GPath *p = gpath_create(&info);
  if (p) {
    gpath_draw_filled(ctx, p);
    gpath_destroy(p);
  }
}

// Balken aus Segmenten wie in Flynformer: er zeigt an, wie weit die Messung
// ist. Ein durchgehender Balken sieht auf E-Paper wie eine Linie aus, die
// Luecken machen den Fortschritt auch aus dem Augenwinkel lesbar.
static void prv_draw_bar(GContext *ctx, GRect box, int done, int total) {
  if (total <= 0) return;
  const int16_t seg_w = (box.size.w - (BAR_SEGS - 1) * BAR_GAP) / BAR_SEGS;
  const int filled = (done * BAR_SEGS) / total;
  for (int i = 0; i < BAR_SEGS; i++) {
    graphics_context_set_fill_color(ctx, i < filled ? HZ_COLOR_BAR : HZ_COLOR_BAR_EMPTY);
    graphics_fill_rect(ctx, GRect(box.origin.x + i * (seg_w + BAR_GAP), box.origin.y,
                                  seg_w, box.size.h), 0, GCornerNone);
  }
}

static void prv_canvas_update(Layer *layer, GContext *ctx) {
  const GRect b = layer_get_bounds(layer);
  const bool wide = PBL_DISPLAY_WIDTH >= 180;
  const int16_t margin = HZ_MARGIN;
  const int16_t col_w = b.size.w - HZ_SIDEBAR_W - margin - 4;
  const HrvPhase phase = hrv_phase();
  const HrvStats *st = hrv_result();

  // Kleine Uhrzeit oben, wie im Kopf eines Timeline-Eintrags
  graphics_context_set_text_color(ctx, HZ_COLOR_TEXT);
  char clock[10];
  clock_copy_time_string(clock, sizeof(clock));
  graphics_draw_text(ctx, clock, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, PBL_IF_ROUND_ELSE(10, 0), b.size.w - HZ_SIDEBAR_W, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  int16_t y = PBL_IF_ROUND_ELSE(46, 18);

  if (phase == HrvNoSensor) {
    graphics_context_set_text_color(ctx, HZ_COLOR_TEXT);
    graphics_draw_text(ctx, S(STR_NO_HRV),
                       fonts_get_system_font(wide ? FONT_KEY_GOTHIC_24_BOLD
                                                  : FONT_KEY_GOTHIC_18_BOLD),
                       GRect(margin, y, col_w, wide ? 30 : 24),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    y += wide ? 32 : 26;
    graphics_context_set_text_color(ctx, HZ_COLOR_DIM);
    graphics_draw_text(ctx, S(STR_NO_HRV_SUB),
                       fonts_get_system_font(wide ? FONT_KEY_GOTHIC_18 : FONT_KEY_GOTHIC_14),
                       GRect(margin, y, col_w, b.size.h - y - 4),
                       GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    return;
  }

  // Im Ruhezustand steht statt des letzten Werts der Hinweis auf den Umzug:
  // Nachtmessung und Einminutenmessung gibt es jetzt in Kieselsport. Die
  // Einminutenmessung hier geht weiter (Mitteltaste), Messung und Ergebnis
  // sehen aus wie bisher - nur wer die App oeffnet, soll erfahren, dass er
  // sie nicht mehr braucht.
  if (phase == HrvIdle) {
    graphics_context_set_text_color(ctx, HZ_COLOR_BIG);
    graphics_draw_text(ctx, S(STR_MOVED),
                       fonts_get_system_font(wide ? FONT_KEY_GOTHIC_24_BOLD
                                                  : FONT_KEY_GOTHIC_18_BOLD),
                       GRect(margin, y, col_w, wide ? 84 : 66),
                       GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    y += wide ? 86 : 68;
    graphics_context_set_text_color(ctx, HZ_COLOR_DIM);
    graphics_draw_text(ctx, S(STR_MOVED_SUB),
                       fonts_get_system_font(wide ? FONT_KEY_GOTHIC_18 : FONT_KEY_GOTHIC_14),
                       GRect(margin, y, col_w, b.size.h - y - 4),
                       GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    return;
  }

  // Kennzeile ueber der grossen Zahl
  const char *label;
  if (phase == HrvMeasuring) label = S(STR_MEASURING);
  else if (phase == HrvDone) label = S(STR_RMSSD);
  else label = hrv_last_rmssd() ? S(STR_LAST) : S(STR_READY);
  graphics_context_set_text_color(ctx, HZ_COLOR_TEXT);
  graphics_draw_text(ctx, label, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(margin, y, col_w, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  y += 14;

  // Die grosse Zahl: Restsekunden waehrend der Messung, sonst der RMSSD.
  char big[8];
  bool have_big = true;
  if (phase == HrvMeasuring) {
    snprintf(big, sizeof(big), "%d", hrv_seconds_left());
  } else if (phase == HrvDone) {
    const uint16_t r = hrv_rmssd_ms(st);
    if (r > 0) snprintf(big, sizeof(big), "%u", r);
    else have_big = false;
  } else if (hrv_last_rmssd() > 0) {
    snprintf(big, sizeof(big), "%u", hrv_last_rmssd());
  } else {
    have_big = false;
  }

  if (have_big) {
    graphics_context_set_text_color(ctx, HZ_COLOR_BIG);
    graphics_draw_text(ctx, big,
                       fonts_get_system_font(wide ? FONT_KEY_LECO_32_BOLD_NUMBERS
                                                  : FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM),
                       GRect(margin, y, col_w, wide ? 38 : 32),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    y += wide ? 42 : 36;
  }

  // Titel und Nebenzeile
  char title[40];
  char sub_buf[40];
  const char *sub = NULL;
  if (phase == HrvMeasuring) {
    if (st->accepted == 0) {
      snprintf(title, sizeof(title), "%s", S(STR_WAITING));
    } else {
      snprintf(title, sizeof(title), S(STR_BEATS_FMT),
               (int)st->accepted, (int)st->rejected);
    }
  } else if (phase == HrvDone) {
    if (hrv_rmssd_ms(st) > 0) {
      snprintf(title, sizeof(title), S(STR_RESULT_FMT),
               (int)hrv_mean_bpm(st), (int)st->accepted);
      // Deckung: wie viel der Messdauer die angenommenen Intervalle zusammen
      // ausfuellen. Bleibt viel uebrig, fehlen Schlaege - entweder weil der
      // Filter sie verworfen hat oder weil der Sensor sie nie gemeldet hat.
      // Die beiden Zahlen nebeneinander sagen, welches von beidem.
      const uint32_t covered = (st->sum_rr / 10) / (HZ_MEASURE_S ? HZ_MEASURE_S : 1);
      snprintf(sub_buf, sizeof(sub_buf), S(STR_QUALITY_FMT),
               (int)st->rejected, (int)(covered > 100 ? 100 : covered));
      sub = sub_buf;
    } else {
      snprintf(title, sizeof(title), "%s", S(STR_TOO_FEW));
      sub = S(STR_TRY_AGAIN);
    }
  } else {
    snprintf(title, sizeof(title), "%s",
             hrv_last_rmssd() ? S(STR_RMSSD) : S(STR_NO_READING));
    sub = S(STR_SIT_STILL);
  }
  graphics_context_set_text_color(ctx, HZ_COLOR_TEXT);
  graphics_draw_text(ctx, title,
                     fonts_get_system_font(wide ? FONT_KEY_GOTHIC_18_BOLD
                                                : FONT_KEY_GOTHIC_14_BOLD),
                     GRect(margin, y, col_w, wide ? 24 : 20),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  y += wide ? 22 : 18;
  if (sub) {
    graphics_context_set_text_color(ctx, HZ_COLOR_DIM);
    graphics_draw_text(ctx, sub,
                       fonts_get_system_font(wide ? FONT_KEY_GOTHIC_18 : FONT_KEY_GOTHIC_14),
                       GRect(margin, y, col_w, 40),
                       GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  }

  // Wie es der Uebergabe ans Telefon ergangen ist. Nur beim Ergebnis, und nur
  // wenn ueberhaupt etwas hinausging - im Ruhezustand waere die Zeile eine
  // Meldung ueber nichts.
  if (phone_status() != PhoneNothing && phase != HrvMeasuring) {
    char line[40];
    switch (phone_status()) {
      case PhoneSent:
        snprintf(line, sizeof(line), "%s", S(STR_TO_PHONE_OK));
        break;
      case PhoneFailed:
        snprintf(line, sizeof(line), S(STR_TO_PHONE_FAIL), phone_fail_reason());
        break;
      default:
        snprintf(line, sizeof(line), "%s", S(STR_TO_PHONE_WAIT));
        break;
    }
    graphics_context_set_text_color(ctx, HZ_COLOR_DIM);
    graphics_draw_text(ctx, line,
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(margin, b.size.h - PBL_IF_ROUND_ELSE(46, 26), col_w, 20),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }

  // Balken unten, nur waehrend der Messung
  if (phase == HrvMeasuring) {
    const int16_t by = b.size.h - BAR_H - PBL_IF_ROUND_ELSE(28, 8);
    prv_draw_bar(ctx, GRect(margin, by, col_w, BAR_H),
                 hrv_duration() - hrv_seconds_left(), hrv_duration());
  }
}

static void prv_sidebar_update(Layer *layer, GContext *ctx) {
  const GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, HZ_COLOR_SIDEBAR);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  const int16_t cx = b.size.w / 2 - HZ_HEART_DX;
  prv_draw_heart(ctx, GPoint(cx, HZ_HEART_Y), HZ_HEART_W);

  // Hinweis auf Hoehe der Mitteltaste - die einzige, die hier etwas tut.
  if (hrv_phase() == HrvNoSensor) return;
  graphics_context_set_text_color(ctx, HZ_COLOR_ON_SIDEBAR);
  graphics_draw_text(ctx, hrv_phase() == HrvMeasuring ? S(STR_HINT_STOP) : S(STR_HINT_START),
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, b.size.h / 2 - 9, b.size.w, 18),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static Layer *s_sidebar;

static void prv_select(ClickRecognizerRef recognizer, void *context) {
  switch (hrv_phase()) {
    case HrvMeasuring:
      hrv_stop();
      break;
    case HrvNoSensor:
      break;          // nichts zu starten
    default:
      hrv_start();
      vibes_short_pulse();
      break;
  }
}

static void prv_click_config(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select);
}

static void prv_tick(struct tm *tick_time, TimeUnits units_changed) {
  main_window_refresh();
}

static void prv_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  const GRect bounds = layer_get_bounds(root);
  s_canvas = layer_create(bounds);
  layer_set_update_proc(s_canvas, prv_canvas_update);
  layer_add_child(root, s_canvas);
  s_sidebar = layer_create(GRect(bounds.size.w - HZ_SIDEBAR_W, 0,
                                 HZ_SIDEBAR_W, bounds.size.h));
  layer_set_update_proc(s_sidebar, prv_sidebar_update);
  layer_add_child(root, s_sidebar);
  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick);
}

static void prv_unload(Window *window) {
  tick_timer_service_unsubscribe();
  layer_destroy(s_sidebar);
  layer_destroy(s_canvas);
  window_destroy(s_window);
  s_window = NULL;
  s_canvas = NULL;
  s_sidebar = NULL;
}

void main_window_refresh(void) {
  if (s_canvas) layer_mark_dirty(s_canvas);
  if (s_sidebar) layer_mark_dirty(s_sidebar);
}

void main_window_push(void) {
  if (s_window) return;
  s_window = window_create();
  window_set_background_color(s_window, HZ_COLOR_BG);
  window_set_click_config_provider(s_window, prv_click_config);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_load, .unload = prv_unload,
  });
  window_stack_push(s_window, true);
}
