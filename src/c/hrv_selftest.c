#include "hrv_selftest.h"

#ifdef HZ_SELFTEST
#include <pebble.h>
#include "hrv_math.h"

static int s_fails;

#define OK(name, cond)                                                     \
  do {                                                                     \
    if (cond) {                                                            \
      APP_LOG(APP_LOG_LEVEL_INFO, "  ok     %s", name);                    \
    } else {                                                               \
      APP_LOG(APP_LOG_LEVEL_ERROR, "  FEHLER %s", name);                   \
      s_fails++;                                                           \
    }                                                                      \
  } while (0)

#define EQ(name, got, want)                                                \
  do {                                                                     \
    const long g_ = (long)(got), w_ = (long)(want);                        \
    if (g_ == w_) {                                                        \
      APP_LOG(APP_LOG_LEVEL_INFO, "  ok     %s (%ld)", name, g_);          \
    } else {                                                               \
      APP_LOG(APP_LOG_LEVEL_ERROR, "  FEHLER %s: %ld statt %ld",           \
              name, g_, w_);                                               \
      s_fails++;                                                           \
    }                                                                      \
  } while (0)

static void prv_feed(HrvStats *s, const uint16_t *rr, int n) {
  hrv_stats_reset(s);
  for (int i = 0; i < n; i++) hrv_stats_add(s, rr[i]);
}

// Unabhaengig bestimmte Erwartung fuer den Vektor unten (Lehrbuchformel:
// RMSSD = 20,0447 ms; die App rundet ab). Quersumme der Quadrate 7634 bei 19
// Differenzen, mittlerer Puls 66,18/min.
static const uint16_t s_vec[] = { 900, 912, 898, 925, 903, 918, 890, 907, 930, 899,
                                  911, 884, 902, 921, 895, 908, 917, 893, 906, 914 };
#define VEC_N        20
#define VEC_SUM_SQ   7634
#define VEC_DIFFS    19
#define VEC_RMSSD    20
#define VEC_BPM      66

int hrv_selftest_run(void) {
  s_fails = 0;
  HrvStats s;

  APP_LOG(APP_LOG_LEVEL_INFO, "== Quadratwurzel ==");
  {
    int bad = 0;
    for (uint32_t v = 0; v < 20000; v++) {
      const uint32_t r = hrv_isqrt(v);
      if ((uint64_t)r * r > v || (uint64_t)(r + 1) * (r + 1) <= v) bad++;
    }
    EQ("0..19999 exakt abgerundet, Fehler", bad, 0);
    EQ("isqrt(0)", hrv_isqrt(0), 0);
    EQ("isqrt(1600)", hrv_isqrt(1600), 40);
    EQ("isqrt(2^32)", hrv_isqrt((uint64_t)1 << 32), 65536);
    // Groesste Summe, die im Betrieb entstehen kann: 7200 Differenzen von je
    // hoechstens 400 ms. Muss ohne Ueberlauf durchgehen.
    EQ("isqrt(7200*400^2)", hrv_isqrt((uint64_t)7200 * 400 * 400), 33941);
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "== Ruhiger Puls ==");
  {
    uint16_t rr[20];
    for (int i = 0; i < 20; i++) rr[i] = 1000;
    prv_feed(&s, rr, 20);
    EQ("alle 20 angenommen", s.accepted, 20);
    EQ("keines verworfen", s.rejected, 0);
    EQ("19 Differenzen", s.diffs, 19);
    EQ("RMSSD ohne Schwankung", hrv_rmssd_ms(&s), 0);
    EQ("Puls 60/min", hrv_mean_bpm(&s), 60);
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "== Gleichmaessige Schwankung ==");
  {
    uint16_t rr[20];
    for (int i = 0; i < 20; i++) rr[i] = (i % 2) ? 1040 : 1000;
    prv_feed(&s, rr, 20);
    EQ("alle angenommen", s.accepted, 20);
    EQ("jede Differenz 40 -> RMSSD 40", hrv_rmssd_ms(&s), 40);
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "== Gegen die vorab bestimmte Erwartung ==");
  {
    prv_feed(&s, s_vec, VEC_N);
    EQ("nichts verworfen", s.rejected, 0);
    EQ("Differenzen", s.diffs, VEC_DIFFS);
    EQ("Summe der Quadrate", (long)s.sum_sq_diff, VEC_SUM_SQ);
    EQ("RMSSD (Lehrbuch 20,04)", hrv_rmssd_ms(&s), VEC_RMSSD);
    EQ("Puls (Lehrbuch 66,18)", hrv_mean_bpm(&s), VEC_BPM);
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "== Unsinn aussortieren ==");
  {
    const uint16_t rr[] = { 1000, 1000, 250, 1000, 5000, 1000, 1000, 1000 };
    prv_feed(&s, rr, 8);
    EQ("zwei unmoegliche Werte verworfen", s.rejected, 2);
    EQ("sechs angenommen", s.accepted, 6);
    // Ketten: (1000,1000) = 1, Bruch, 1000 allein, Bruch, (1000,1000,1000) = 2
    EQ("Differenzen nur ueber ununterbrochene Paare", s.diffs, 3);
    EQ("zu wenig Differenzen -> kein RMSSD", hrv_rmssd_ms(&s), 0);
  }
  {
    // Ausgelassener Schlag: fast doppelte Laenge, im Fenster, aber ueber 20 %.
    const uint16_t rr[] = { 900, 900, 900, 1800, 900, 900, 900 };
    prv_feed(&s, rr, 7);
    EQ("der doppelte Schlag ist draussen", s.rejected, 1);
    EQ("sechs angenommen", s.accepted, 6);
    EQ("2 Differenzen davor, 2 danach", s.diffs, 4);
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "== Filter bleibt nicht haengen ==");
  {
    // Puls steigt schnell. Ohne Neuansetzen verwuerfe der Filter ab hier alles.
    const uint16_t rr[] = { 1000, 1000, 1000, 600, 600, 600, 600, 600, 600, 600, 600 };
    prv_feed(&s, rr, 11);
    EQ("angenommen", s.accepted, 9);
    EQ("verworfen (die ersten beiden 600)", s.rejected, 2);
    OK("Puls liegt zwischen beiden Werten",
       hrv_mean_bpm(&s) > 60 && hrv_mean_bpm(&s) < 100);
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "== Zu wenig Daten ==");
  {
    hrv_stats_reset(&s);
    EQ("frisch: RMSSD", hrv_rmssd_ms(&s), 0);
    EQ("frisch: Puls", hrv_mean_bpm(&s), 0);
    for (int i = 0; i < HRV_MIN_DIFFS; i++) hrv_stats_add(&s, 1000);
    EQ("eine Differenz zu wenig", hrv_rmssd_ms(&s), 0);
    hrv_stats_add(&s, 1040);
    OK("genug Differenzen: jetzt ein Wert", hrv_rmssd_ms(&s) > 0);
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "== Raender ==");
  {
    hrv_stats_reset(&s);
    OK("genau an der Untergrenze angenommen", hrv_stats_add(&s, HRV_RR_MIN_MS));
    hrv_stats_reset(&s);
    OK("genau an der Obergrenze angenommen", hrv_stats_add(&s, HRV_RR_MAX_MS));
    hrv_stats_reset(&s);
    OK("eins darunter verworfen", !hrv_stats_add(&s, HRV_RR_MIN_MS - 1));
    hrv_stats_reset(&s);
    OK("eins darueber verworfen", !hrv_stats_add(&s, HRV_RR_MAX_MS + 1));
    hrv_stats_reset(&s);
    OK("Null verworfen", !hrv_stats_add(&s, 0));
    OK("NULL-Zeiger stuerzt nicht ab", !hrv_stats_add(NULL, 1000));
    EQ("RMSSD von NULL", hrv_rmssd_ms(NULL), 0);
    EQ("Puls von NULL", hrv_mean_bpm(NULL), 0);

    hrv_stats_reset(&s);
    hrv_stats_add(&s, 1000);
    OK("genau 20 % noch angenommen", hrv_stats_add(&s, 1200));
    hrv_stats_reset(&s);
    hrv_stats_add(&s, 1000);
    OK("knapp darueber verworfen", !hrv_stats_add(&s, 1201));
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "== Dauerbetrieb, zwei Stunden ==");
  {
    // 900,910,...,960,900,... - der Ruecksprung betraegt 6,7 % und bleibt
    // damit unter der Schwelle, es wird also nichts verworfen. Vorab bestimmt:
    // Summe 6695940 ms, Quadratsumme 4317900 bei 7199 Differenzen,
    // RMSSD 24 (Lehrbuch 24,49), Puls 64/min.
    hrv_stats_reset(&s);
    for (int i = 0; i < 7200; i++) hrv_stats_add(&s, (uint16_t)(900 + (i % 7) * 10));
    EQ("angenommen", s.accepted, 7200);
    EQ("verworfen", s.rejected, 0);
    EQ("Summe der Intervalle", (long)s.sum_rr, 6695940);
    EQ("Summe der Quadrate", (long)s.sum_sq_diff, 4317900);
    EQ("Differenzen", s.diffs, 7199);
    EQ("RMSSD (Lehrbuch 24,49)", hrv_rmssd_ms(&s), 24);
    EQ("Puls", hrv_mean_bpm(&s), 64);
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "== SELBSTTEST FERTIG, Fehler: %d ==", s_fails);
  return s_fails;
}
#else
// Ohne den Schalter bleibt diese Uebersetzungseinheit leer. Ein leeres C-File
// ist nicht ueberall zulaessig, deshalb ein Platzhalter.
typedef int hrv_selftest_not_built;
#endif
