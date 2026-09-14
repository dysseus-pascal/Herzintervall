#include "hrv_math.h"

void hrv_stats_reset(HrvStats *s) {
  if (!s) return;
  s->prev_rr = 0;
  s->chain = false;
  s->rejects_row = 0;
  s->accepted = 0;
  s->rejected = 0;
  s->sum_rr = 0;
  s->diffs = 0;
  s->sum_sq_diff = 0;
}

// Uebernehmen, ohne eine Differenz zu bilden. Dafuer gibt es zwei Anlaesse:
// das allererste Intervall, und das Neuansetzen nach einer Reihe verworfener.
// In beiden Faellen steht kein unmittelbarer Vorgaenger daneben, und eine
// Differenz ueber die Luecke hinweg waere erfunden.
static void prv_anchor(HrvStats *s, uint16_t rr_ms) {
  s->accepted++;
  s->sum_rr += rr_ms;
  s->prev_rr = rr_ms;
  s->chain = true;
  s->rejects_row = 0;
}

static void prv_reject(HrvStats *s) {
  s->rejected++;
  s->chain = false;   // die Kette ist unterbrochen, egal wie es weitergeht
  if (s->rejects_row < 255) s->rejects_row++;
}

bool hrv_stats_add(HrvStats *s, uint16_t rr_ms) {
  if (!s) return false;

  // 1. Plausibles Fenster
  if (rr_ms < HRV_RR_MIN_MS || rr_ms > HRV_RR_MAX_MS) {
    prv_reject(s);
    return false;
  }

  // 2. Erstes Intervall: nichts zu vergleichen
  if (s->prev_rr == 0) {
    prv_anchor(s, rr_ms);
    return true;
  }

  // 3. Sprung zum Vorgaenger. In ganzen Zahlen gerechnet, ohne Division:
  //    |rr - prev| * 100 > prev * HRV_JUMP_PERCENT
  const int32_t diff = (int32_t)rr_ms - (int32_t)s->prev_rr;
  const uint32_t adiff = (uint32_t)(diff < 0 ? -diff : diff);
  if (adiff * 100u > (uint32_t)s->prev_rr * HRV_JUMP_PERCENT) {
    prv_reject(s);
    // Haengt der Filter? Dann liegt es eher am Puls als am Sensor - neu
    // ansetzen, aber ohne Differenz ueber die Luecke.
    if (s->rejects_row >= HRV_REANCHOR_AFTER) {
      prv_anchor(s, rr_ms);
      s->rejected--;   // dieses Intervall zaehlt nun doch als angenommen
      return true;
    }
    return false;
  }

  // 4. Angenommen. Eine Differenz entsteht nur, wenn der Vorgaenger wirklich
  //    der unmittelbar vorhergehende Schlag war.
  if (s->chain) {
    s->sum_sq_diff += (uint64_t)((int64_t)diff * (int64_t)diff);
    s->diffs++;
  }
  s->accepted++;
  s->sum_rr += rr_ms;
  s->prev_rr = rr_ms;
  s->chain = true;
  s->rejects_row = 0;
  return true;
}

uint32_t hrv_isqrt(uint64_t v) {
  // Ziffernweise binaer, ohne Schleifenabbruch nach Gefuehl: hoechstes
  // gesetztes Bit suchen, dann Bit fuer Bit pruefen.
  if (v == 0) return 0;
  uint64_t rest = v, root = 0, bit = 1ULL << 62;
  while (bit > rest) bit >>= 2;
  while (bit != 0) {
    if (rest >= root + bit) {
      rest -= root + bit;
      root = (root >> 1) + bit;
    } else {
      root >>= 1;
    }
    bit >>= 2;
  }
  return (uint32_t)root;
}

uint16_t hrv_rmssd_ms(const HrvStats *s) {
  if (!s || s->diffs < HRV_MIN_DIFFS) return 0;
  const uint64_t mean_sq = s->sum_sq_diff / s->diffs;
  const uint32_t r = hrv_isqrt(mean_sq);
  return (r > 0xFFFF) ? 0xFFFF : (uint16_t)r;
}

uint16_t hrv_mean_bpm(const HrvStats *s) {
  if (!s || s->accepted == 0 || s->sum_rr == 0) return 0;
  // 60000 ms je Minute, geteilt durch das mittlere Intervall. Zaehler und
  // Nenner so gestellt, dass nur einmal gerundet wird.
  const uint32_t bpm = (60000u * s->accepted) / s->sum_rr;
  return (bpm > 0xFFFF) ? 0xFFFF : (uint16_t)bpm;
}
