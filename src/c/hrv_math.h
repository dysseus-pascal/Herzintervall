#pragma once
#include <stdbool.h>
#include <stdint.h>

// Auswertung der Schlagintervalle - reine Rechnung, KEIN pebble.h.
//
// Diese Datei haengt bewusst an nichts: sie laesst sich auf dem Baurechner mit
// gcc uebersetzen und pruefen (tools/hrv_math_test.c). Das ist hier nicht
// Ordnungsliebe, sondern Notwendigkeit - der Sensor der Uhr ist im Emulator
// nicht nachgebildet (CONFIG_HRM_HRV steht nur im Board obelix, nicht in
// qemu_emery). Was am Handgelenk passiert, kann niemand hier nachstellen; was
// gerechnet wird, muss deshalb umso haerter belegt sein.
//
// GRUND DER FILTERUNG: die Uhr liefert jedes Intervall einzeln, aber OHNE
// seine Guete - der Health-Handler bekommt nur den Ereignistyp, und die
// Struktur mit dem Konfidenzwert ist in der Firmware als @internal markiert.
// Ein verschlucktes oder doppelt gezaehltes Herzschlag-Intervall ist damit
// nicht am Wert zu erkennen, und RMSSD reagiert auf genau solche Ausreisser
// heftiger als auf alles andere. Also wird statistisch aussortiert.

// Plausibles Fenster: 300 ms = 200/min, 2000 ms = 30/min. Alles ausserhalb ist
// kein Herzschlag, sondern ein Messfehler.
#define HRV_RR_MIN_MS 300
#define HRV_RR_MAX_MS 2000

// Ein Intervall darf vom vorigen hoechstens so viel Prozent abweichen. 20 % ist
// der uebliche Wert der Artefaktkorrektur: echte Schlag-zu-Schlag-Schwankung
// bleibt darunter, ein ausgelassener Schlag (fast doppelte Laenge) darueber.
#define HRV_JUMP_PERCENT 20

// Nach so vielen verworfenen Intervallen in Folge wird neu angesetzt. Ohne das
// bliebe der Filter haengen, sobald sich der Puls schnell aendert: er vergliche
// ewig gegen einen alten Wert und verwuerfe jeden neuen.
#define HRV_REANCHOR_AFTER 3

// Weniger Differenzen als das ergeben keinen sinnvollen RMSSD.
#define HRV_MIN_DIFFS 5

typedef struct {
  uint16_t prev_rr;      //< letztes ANGENOMMENES Intervall, 0 = noch keines
  bool chain;            //< steht prev_rr unmittelbar vor dem naechsten Schlag?
  uint8_t rejects_row;   //< verworfene in Folge, fuer das Neuansetzen
  uint32_t accepted;
  uint32_t rejected;
  uint32_t sum_rr;       //< Summe der angenommenen Intervalle in ms
  uint32_t diffs;        //< Anzahl gueltiger Differenzen aufeinanderfolgender Schlaege
  uint64_t sum_sq_diff;  //< Summe der quadrierten Differenzen
} HrvStats;

void hrv_stats_reset(HrvStats *s);

// Ein Intervall einspeisen. Rueckgabe: true, wenn es angenommen wurde.
bool hrv_stats_add(HrvStats *s, uint16_t rr_ms);

// RMSSD in ms, 0 wenn noch zu wenig Daten (< HRV_MIN_DIFFS Differenzen).
uint16_t hrv_rmssd_ms(const HrvStats *s);

// Mittlere Herzfrequenz in Schlaegen je Minute, 0 wenn nichts angenommen wurde.
uint16_t hrv_mean_bpm(const HrvStats *s);

// Ganzzahlige Quadratwurzel, abgerundet. Eigen, weil die Uhr keine
// Fliesskomma-Einheit hat und sqrt() dort teuer erkauft waere.
uint32_t hrv_isqrt(uint64_t v);
