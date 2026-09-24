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
//   STR(schluessel, maxbytes, en, de, fr, it, es)
//
//   maxbytes  Groesse des Zielpuffers in BYTES, 0 wenn der Text in keinen
//             festen Puffer kopiert wird. Umlaute zaehlen als zwei Bytes.
//             tools/strings_check.js prueft diese Grenze.
//   en        Englisch. Spalte 0 und zugleich der Rueckfall fuer jede Sprache,
//             die hier keine eigene Spalte hat.
//   de        Deutsch.
//   fr        Franzoesisch.
//   it        Italienisch.
//   es        Spanisch.
//
// Ein Budget von n Bytes heisst: hoechstens n-1 Bytes Text, das letzte
// braucht die abschliessende Null. Akzente zaehlen wie Umlaute doppelt.

// --- Hauptscreen, Ruhezustand ---
STR(STR_APP_NAME,      0,  "Herzintervall",     "Herzintervall", "Herzintervall", "Herzintervall", "Herzintervall")
STR(STR_READY,         0,  "Ready",             "Bereit", "Prêt", "Pronto", "Listo")
STR(STR_NO_READING,    0,  "No reading yet",    "Noch keine Messung", "Aucune mesure", "Nessuna misura", "Sin medición")
STR(STR_LAST,          0,  "Last measurement",  "Letzte Messung", "Dernière mesure", "Ultima misura", "Última medición")
STR(STR_SIT_STILL,     0,  "Sit still, then press",  "Ruhig sitzen, dann drücken", "Reste assis, puis appuie", "Stai fermo, poi premi", "Quédate quieto y pulsa")

// --- Umzug nach Kieselsport ---
// Steht im Ruhezustand statt des letzten Werts. Kurz, weil flint nur rund
// 100 px Spaltenbreite hat; der Titel darf dort auf drei Zeilen umbrechen.
STR(STR_MOVED,         0,  "Now part of Kieselsport", "Jetzt Teil von Kieselsport", "Désormais dans Kieselsport", "Ora parte di Kieselsport", "Ahora parte de Kieselsport")
STR(STR_MOVED_SUB,     0,  "This app can be deleted.", "Diese App kann gelöscht werden.", "Cette app peut être supprimée.", "Questa app può essere eliminata.", "Esta app se puede borrar.")

// --- Messung laeuft ---
STR(STR_MEASURING,     0,  "Measuring",         "Messung läuft", "Mesure en cours", "Misura in corso", "Midiendo")
STR(STR_WAITING,       0,  "Waiting for beats", "Warte auf Schläge", "Attente du pouls", "Attendo i battiti", "Esperando latidos")
STR(STR_BEATS_FMT,    40,  "%d beats, %d dropped", "%d Schläge, %d raus", "%d batt., %d écartés", "%d battiti, %d via", "%d latidos, %d fuera")

// --- Ergebnis ---
STR(STR_RMSSD,         0,  "RMSSD in ms",       "RMSSD in ms", "RMSSD en ms", "RMSSD in ms", "RMSSD en ms")
STR(STR_RESULT_FMT,   40,  "%d bpm, %d beats",  "%d/min, %d Schläge", "%d/min, %d batt.", "%d/min, %d battiti", "%d/min, %d latidos")
STR(STR_TOO_FEW,       0,  "Too few clean beats", "Zu wenig saubere Schläge", "Trop peu de battements", "Pochi battiti validi", "Pocos latidos válidos")
// Wie viel von der Minute wirklich abgedeckt ist. Ohne diese Zeile sieht man
// dem Ergebnis nicht an, ob der Filter viel wegwerfen musste oder ob der
// Sensor gar nicht erst geliefert hat - und das sind zwei verschiedene Dinge.
STR(STR_QUALITY_FMT,  40,  "%d dropped, %d%% covered", "%d verworfen, %d%% gedeckt", "%d écartés, %d%% couvert", "%d scartati, %d%% coperto", "%d descartados, %d%% cubierto")
STR(STR_TRY_AGAIN,     0,  "Sit still and retry",  "Ruhig sitzen, nochmal", "Reste immobile et réessaie", "Stai fermo e riprova", "Quédate quieto y repite")

// --- Uebergabe ans Telefon ---
// Steht auf dem Ergebnisschirm, damit sich ohne Rechner am Kabel feststellen
// laesst, wo es klemmt. "Uebergeben" heisst dabei: die Companion-App hat den
// Empfang bestaetigt, nicht bloss "abgeschickt".
STR(STR_TO_PHONE_WAIT, 0,  "sending…",          "wird gesendet…", "envoi…", "invio…", "enviando…")
STR(STR_TO_PHONE_OK,   0,  "handed to phone",   "ans Telefon übergeben", "remis au téléphone", "arrivato al telefono", "entregado al móvil")
STR(STR_TO_PHONE_FAIL,32,  "phone not reached (%d)", "Telefon nicht erreicht (%d)", "téléphone injoignable (%d)", "telefono non raggiunto (%d)", "móvil no disponible (%d)")

// --- Kein HRV ---
// EINE Meldung fuer beide Ursachen, weil die Uhr sie nicht unterscheidbar
// macht: health_service_set_hrv_sample_period() gibt false zurueck, wenn der
// Sensor fehlt UND wenn die Herzfrequenz in den Einstellungen abgeschaltet
// ist. Zwei Meldungen waeren geraten, nicht gewusst.
STR(STR_NO_HRV,        0,  "No HRV",            "Kein HRV", "Pas de HRV", "Nessun HRV", "Sin HRV")
STR(STR_NO_HRV_SUB,    0,  "This watch delivers no beat intervals. On a Pebble Time 2, switch heart rate on in the settings.", "Diese Uhr liefert keine Schlagintervalle. Auf der Pebble Time 2: Herzfrequenz in den Einstellungen einschalten.", "Cette montre ne fournit pas d'intervalles entre battements. Sur une Pebble Time 2, active la fréquence cardiaque dans les réglages.", "Questo orologio non fornisce intervalli tra i battiti. Su un Pebble Time 2, attiva la frequenza cardiaca nelle impostazioni.", "Este reloj no da intervalos entre latidos. En un Pebble Time 2, activa la frecuencia cardiaca en los ajustes.")

// --- Tastenhinweise in der Seitenleiste (kurz! die Leiste ist schmal) ---
STR(STR_HINT_START,   14,  "Start",             "Start", "Lancer", "Avvia", "Medir")
STR(STR_HINT_STOP,    14,  "Stop",              "Stopp", "Stop", "Stop", "Parar")

// --- App-Glance im Starter ---
STR(STR_GLANCE_FMT,   48,  "RMSSD %d ms",       "RMSSD %d ms", "RMSSD %d ms", "RMSSD %d ms", "RMSSD %d ms")
STR(STR_GLANCE_NONE,  48,  "No reading yet",    "Noch keine Messung", "Aucune mesure", "Nessuna misura", "Sin medición")
