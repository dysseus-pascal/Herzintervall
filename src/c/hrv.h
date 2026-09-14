#pragma once
#include <pebble.h>
#include "hrv_math.h"

// Die Messung: Sensor anfordern, Schlagintervalle einsammeln, nach
// HZ_MEASURE_S Sekunden auswerten.
//
// WAS DIE UHR HERGIBT - und was nicht:
//   * Jedes Intervall kommt als eigenes Ereignis (HealthEventHRVUpdate). Der
//     Wert wird mit health_service_peek_hrv_ppi_ms() geholt.
//   * Der Ereignis-Handler bekommt NUR den Typ, kein Datenpaket. Die Guete des
//     Intervalls ist in der Firmware als @internal markiert und fuer Apps
//     unsichtbar. Deshalb filtert hrv_math.c statistisch nach.
//   * Nichts davon wird gespeichert. Laeuft die App nicht, wird nicht
//     gesammelt - es gibt keinen Verlauf in der Health-Datenbank.
//   * Ausserhalb des Handgelenks liefert die Firmware ein Intervall 0, also
//     dasselbe wie "noch nichts gemessen". Beides ist nicht unterscheidbar.

// Dauer einer von Hand gestarteten Messung. Eine Minute ist kurz - der
// Lehrbuchstandard fuer Kurzzeit-HRV sind fuenf -, aber RMSSD haelt das besser
// aus als andere Kennzahlen, und laenger still zu sitzen haelt niemand durch.
#define HZ_MEASURE_S 60

// Die naechtliche Messung laeuft laenger: im Schlaf stoert die Dauer nicht,
// und drei Minuten liefern deutlich mehr Intervallpaare als eine. Genau das
// macht den Wert mit sich selbst vergleichbar - was bei HRV allein zaehlt.
#ifdef HZ_TEST_NIGHT
// Pruefbau: kurz genug, um den ganzen Weckpfad im Emulator durchzuspielen.
#define HZ_NIGHT_S 20
#else
#define HZ_NIGHT_S 180
#endif

// Wann sie stattfindet, volle Stunde in Ortszeit.
#define HZ_NIGHT_HOUR 5

// Abtastperiode in Sekunden, die die App anfordert. 1 = so dicht wie erlaubt;
// der Sensor laeuft waehrend der Messung durchgehend, was er nur tut, solange
// eine App eine Periode haelt.
#define HZ_SAMPLE_S 1

typedef enum {
  HrvIdle,        //< bereit, evtl. mit einem Ergebnis von vorhin
  HrvMeasuring,
  HrvDone,        //< Messung beendet, Ergebnis steht
  HrvNoSensor,    //< die Uhr liefert keine Intervalle
} HrvPhase;

// Wird gerufen, wenn sich etwas Sichtbares geaendert hat.
typedef void (*HrvChanged)(void);

void hrv_init(HrvChanged on_change);

// Sensor freigeben. MUSS beim Beenden gerufen werden: eine gehaltene
// Abtastperiode laesst den Sensor sonst weiterlaufen und kostet Batterie.
void hrv_deinit(void);

// Messung starten. Schlaegt fehl, wenn die Uhr kein HRV liefert; die Phase
// steht danach auf HrvNoSensor.
bool hrv_start(void);

// Dasselbe mit der Dauer der Nachtmessung.
bool hrv_start_night(void);

// Dauer der laufenden (oder zuletzt gelaufenen) Messung in Sekunden. Die
// Deckung rechnet sich daraus - mit der falschen Dauer waere sie Unsinn.
int hrv_duration(void);

// Lief die letzte Messung als Nachtmessung?
bool hrv_was_night(void);

// Vorzeitig abbrechen und auswerten, was da ist.
void hrv_stop(void);

HrvPhase hrv_phase(void);
const HrvStats *hrv_result(void);
int hrv_seconds_left(void);

// Letztes Ergebnis (ueber Neustarts hinweg gemerkt), 0 wenn es keines gibt.
uint16_t hrv_last_rmssd(void);
time_t hrv_last_time(void);
