// Alle Texte der Oberflaeche, eine Zeile je Text.
//
// ACHTUNG, ZWEI DINGE SIND ABSICHT:
//  1. KEIN #pragma once und keine Include-Waechter. Diese Datei wird MEHRFACH
//     eingebunden (X-Makro): einmal fuer die Aufzaehlung der Schluessel in
//     strings.h und einmal fuer die Tabelle in strings.c. Ein Waechter wuerde
//     die zweite Einbindung verschlucken und eine leere Tabelle erzeugen.
//  2. Endung .h, obwohl es kein gewoehnlicher Header ist. In Drinktervall hiess
//     sie frueher .def; Build-Umgebungen, die nur .c und .h in ihren Baum
//     kopieren, haben sie dann nicht gefunden.
//
//   STR(schluessel, maxbytes, en, de)
//
//   maxbytes  Groesse des Zielpuffers in BYTES, 0 wenn der Text in keinen
//             festen Puffer kopiert wird. Umlaute zaehlen als zwei Bytes.
//             tools/strings_check.js prueft diese Grenze.
//   en        Englisch. Spalte 0 und zugleich der Rueckfall fuer jede Sprache,
//             die hier keine eigene Spalte hat.
//   de        Deutsch.

// --- Hauptscreen, Ruhezustand ---
STR(STR_APP_NAME,      0,  "Herzintervall",     "Herzintervall")
STR(STR_READY,         0,  "Ready",             "Bereit")
STR(STR_NO_READING,    0,  "No reading yet",    "Noch keine Messung")
STR(STR_LAST,          0,  "Last measurement",  "Letzte Messung")
STR(STR_SIT_STILL,     0,  "Sit still, then press",  "Ruhig sitzen, dann drücken")

// --- Messung laeuft ---
STR(STR_MEASURING,     0,  "Measuring",         "Messung läuft")
STR(STR_WAITING,       0,  "Waiting for beats", "Warte auf Schläge")
STR(STR_BEATS_FMT,    40,  "%d beats, %d dropped", "%d Schläge, %d raus")

// --- Ergebnis ---
STR(STR_RMSSD,         0,  "RMSSD in ms",       "RMSSD in ms")
STR(STR_RESULT_FMT,   40,  "%d bpm, %d beats",  "%d/min, %d Schläge")
STR(STR_TOO_FEW,       0,  "Too few clean beats", "Zu wenig saubere Schläge")
// Wie viel von der Minute wirklich abgedeckt ist. Ohne diese Zeile sieht man
// dem Ergebnis nicht an, ob der Filter viel wegwerfen musste oder ob der
// Sensor gar nicht erst geliefert hat - und das sind zwei verschiedene Dinge.
STR(STR_QUALITY_FMT,  40,  "%d dropped, %d%% covered", "%d verworfen, %d%% gedeckt")
STR(STR_TRY_AGAIN,     0,  "Sit still and retry",  "Ruhig sitzen, nochmal")

// --- Kein HRV ---
// EINE Meldung fuer beide Ursachen, weil die Uhr sie nicht unterscheidbar
// macht: health_service_set_hrv_sample_period() gibt false zurueck, wenn der
// Sensor fehlt UND wenn die Herzfrequenz in den Einstellungen abgeschaltet
// ist. Zwei Meldungen waeren geraten, nicht gewusst.
STR(STR_NO_HRV,        0,  "No HRV",            "Kein HRV")
STR(STR_NO_HRV_SUB,    0,  "This watch delivers no beat intervals. On a Pebble Time 2, switch heart rate on in the settings.", "Diese Uhr liefert keine Schlagintervalle. Auf der Pebble Time 2: Herzfrequenz in den Einstellungen einschalten.")

// --- Tastenhinweise in der Seitenleiste (kurz! die Leiste ist schmal) ---
STR(STR_HINT_START,   14,  "Start",             "Start")
STR(STR_HINT_STOP,    14,  "Stop",              "Stopp")

// --- App-Glance im Starter ---
STR(STR_GLANCE_FMT,   48,  "RMSSD %d ms",       "RMSSD %d ms")
STR(STR_GLANCE_NONE,  48,  "No reading yet",    "Noch keine Messung")
