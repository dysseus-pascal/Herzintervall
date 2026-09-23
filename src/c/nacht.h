#pragma once
#include <pebble.h>

// Die letzte VOLLSTAENDIGE Nacht, wie die Uhr sie gesehen hat - fuer das
// Telefon.
//
// WARUM HIER: Herzintervall misst nachts und redet morgens mit dem Telefon;
// Kiesel-Helper hoert mit und traegt ein, was in die Gesundheitsakte gehoert.
// Schlaf und Ruhepuls fahren beim Ergebnis der Messung einfach mit.
//
// UM FUENF SCHLAEFT MAN NOCH. Die Nacht, die gerade laeuft, ist nicht zu
// Ende; geschickt wird deshalb die letzte abgeschlossene - meist die von
// gestern. Sie kommt einen Tag versetzt an, aber mit ihren echten Zeiten, und
// in der Akte steht sie am richtigen Tag.

typedef struct {
  time_t beginn;       //< 0 = keine Nacht gefunden
  time_t ende;
  uint32_t erholsam_s; //< erholsame Sekunden darin
  uint16_t ruhepuls;   //< mittlerer Puls dieser Nacht, 0 = keiner zu haben
} Nacht;

Nacht nacht_lesen(void);
