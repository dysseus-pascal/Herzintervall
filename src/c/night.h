#pragma once
#include <pebble.h>

// Die naechtliche Messung: jeden Tag um HZ_NIGHT_HOUR, drei Minuten lang.
//
// WARUM NACHTS: HRV im Schlaf ist der Wert, den auch Garmin, Whoop und Oura
// ausweisen. Er haengt weniger an Haltung, Atmung und Tagesform als eine
// wache Minute im Sitzen - und ist damit von Tag zu Tag vergleichbar, was bei
// HRV das einzige ist, was zaehlt.
//
// WAS MAN DABEI WISSEN MUSS: ein Pebble-Wakeup startet die App im
// VORDERGRUND. Es gibt keinen stillen Hintergrundlauf; der Worker-Prozess
// kaeme zwar an den Sensor, aber nicht an AppMessage. Um fuenf Uhr geht also
// kurz der Schirm an. Danach beendet sich die App von selbst wieder.

// SEIT 0.9.0 STILLGELEGT: die Nachtmessung macht jetzt Kieselsport (siehe
// herzintervall.c). night_schedule() wird nicht mehr gerufen; die Datei
// bleibt nur, damit ein alter Wecker noch erkannt wird.

// Naechsten Wecker stellen. Bei jedem Start rufen - dann haelt er sich
// selbst aktuell, auch ueber Zeitumstellungen und Neustarts hinweg.
void night_schedule(void);

// Zeitpunkt des naechsten Weckers, 0 wenn keiner steht.
time_t night_next(void);

// Wurde die App von diesem Wecker gestartet?
bool night_launched_us(void);
