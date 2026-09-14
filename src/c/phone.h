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
// JavaScript-Teil reicht die Pebble-App eingehende AppMessages an eine
// Companion-App weiter. Beides zugleich geht nicht: "PebbleKit JS cannot be
// used in conjunction with PebbleKit Android or PebbleKit iOS."
//
// UND KEIN companionApp-EINTRAG IN package.json. Das klingt verkehrt herum,
// ist aber genau richtig - CompanionAppLifecycleManager.android.kt entscheidet
// so:
//
//   hasAnyPebbleKit2CompanionApps = companionApp?.android?.apps?.any { pkg != null }
//   if (hasAnyPebbleKit2CompanionApps) PebbleKit2(...) else PebbleKitClassic(...)
//
// Ein Eintrag dort schaltet also auf PebbleKit2 um, und das BINDET SICH AN
// EINEN DIENST in der Companion-App, statt zu senden. Wer wie wir den
// klassischen Broadcast empfaengt, darf dort nicht stehen. Genau daran lag es:
// die Uhr meldete APP_MSG_SEND_TIMEOUT, weil sich die Pebble-App an einen
// Dienst zu binden versuchte, den es nicht gibt.
void phone_init(void);

// Ein fertiges Ergebnis schicken.
//
// Schlaegt es fehl - Telefon nicht verbunden, Companion nicht installiert -,
// ist das kein Drama: der Wert steht weiter auf der Uhr, und die naechste
// Messung geht wieder hinaus. Ein Wiederholversuch waere hier mehr Aufwand,
// als die Sache wert ist; eine verpasste Einzelmessung ist kein Verlust, der
// sich nicht durch nochmal Messen beheben liesse.
void phone_send_result(const HrvStats *st, uint16_t rmssd_ms, time_t when);

// Wie es der letzten Uebergabe ergangen ist. Steht auf dem Ergebnisschirm,
// weil es sonst niemand erfahren kann: Uhr-Logs liest man nur mit einem
// Rechner am Kabel, und genau der fehlt unterwegs.
//
// WAS EIN "UEBERGEBEN" BEDEUTET - UND WAS NICHT: PhoneSent heisst, die
// TELEFONSEITE hat bestaetigt. Es heisst NICHT, dass die Companion-App etwas
// gesehen hat. Im Emulator steht die Zeile auf "uebergeben", obwohl dort gar
// keine Companion-App existiert - die Telefon-Nachbildung bestaetigt von sich
// aus. Dieselbe Annahme stand hier zuerst falsch im Kommentar, bis der erste
// Emulatorlauf sie widerlegt hat.
//
// Brauchbar ist die Anzeige trotzdem, sogar sehr:
//   PhoneFailed  -> es klemmt zwischen Uhr und Telefon. Sicherer Befund.
//   PhoneSent    -> das Telefon hat es. Ob die Companion-App es bekam, sagt
//                   allein DEREN Bildschirm. Damit sind die beiden moeglichen
//                   Fehlerstellen sauber getrennt, ohne ein einziges Log.
typedef enum {
  PhoneNothing = 0,   //< noch nichts geschickt
  PhoneSending,       //< unterwegs
  PhoneSent,          //< von der Gegenseite bestaetigt
  PhoneFailed,        //< keine Bestaetigung
} PhoneStatus;

PhoneStatus phone_status(void);

// Grund des Fehlschlags (AppMessageResult), 0 wenn keiner. Die Zahl steht mit
// auf dem Schirm: APP_MSG_SEND_TIMEOUT (64) heisst "keine Bestaetigung" und
// ist etwas anderes als APP_MSG_NOT_CONNECTED (32) - "gar kein Telefon da".
int phone_fail_reason(void);

// Wird gerufen, wenn sich der Zustand aendert.
void phone_set_observer(void (*on_change)(void));
