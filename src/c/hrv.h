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

#define HZ_MEASURE_S 60    //< Dauer einer Messung in Sekunden

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

// Vorzeitig abbrechen und auswerten, was da ist.
void hrv_stop(void);

HrvPhase hrv_phase(void);
const HrvStats *hrv_result(void);
int hrv_seconds_left(void);

// Letztes Ergebnis (ueber Neustarts hinweg gemerkt), 0 wenn es keines gibt.
uint16_t hrv_last_rmssd(void);
time_t hrv_last_time(void);
