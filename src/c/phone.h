#pragma once
#include <pebble.h>
#include "hrv_math.h"

// Das Ergebnis ans Telefon reichen, damit die Companion-App es in Health
// Connect schreiben kann (siehe companion/).
//
// WARUM OHNE WECKER: die Messung passiert bei geoeffneter App. Genau dann ist
// die Verbindung zum Telefon ohnehin da - der Wert geht also sofort hinaus,
// und die Uhr muss nie von sich aus aufwachen.
//
// WOHIN DIE NACHRICHT GEHT: diese App hat KEIN src/pkjs/index.js. Ohne
// JavaScript-Teil reicht die Pebble-App eingehende AppMessages an die
// registrierten Companion-Apps weiter (package.json -> companionApp.android).
// Beides zugleich geht nicht: "PebbleKit JS cannot be used in conjunction with
// PebbleKit Android or PebbleKit iOS." Ein index.js hier waere also nicht
// bloss ueberfluessig, es wuerde die Companion-App abklemmen.
void phone_init(void);

// Ein fertiges Ergebnis schicken.
//
// Schlaegt es fehl - Telefon nicht verbunden, Companion nicht installiert -,
// ist das kein Drama: der Wert steht weiter auf der Uhr, und die naechste
// Messung geht wieder hinaus. Ein Wiederholversuch waere hier mehr Aufwand,
// als die Sache wert ist; eine verpasste Einzelmessung ist kein Verlust, der
// sich nicht durch nochmal Messen beheben liesse.
void phone_send_result(const HrvStats *st, uint16_t rmssd_ms, time_t when);
