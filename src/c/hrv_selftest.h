#pragma once

// Selbsttest der Rechnung in hrv_math.c, ausgefuehrt AUF DER UHR.
//
// Warum nicht auf dem Baurechner? Weil dort kein C-Compiler steht (kein gcc,
// kein sudo fuer apt) - nur der ARM-Compiler der SDK. Ein Test, der nie laeuft,
// ist keiner. Also laeuft er dort, wo ohnehin uebersetzt wird: im Emulator, auf
// derselben 32-Bit-ARM-Zielarchitektur wie auf der echten Uhr. Das prueft
// nebenbei mehr als ein Lauf auf x86-64 - Typbreiten und Ausrichtung stimmen
// dann nachweislich auch dort.
//
// Die Erwartungswerte sind NICHT zur Laufzeit nachgerechnet, sondern vorher
// unabhaengig bestimmt (Lehrbuchformel in Fliesskomma) und hier als Konstanten
// eingetragen. Ein Test, der seine Erwartung aus demselben Code zieht, den er
// prueft, bestaetigt nur sich selbst.
//
// Gebaut wird er nur mit -DHZ_SELFTEST; im ausgelieferten Paket ist kein Byte
// davon. Siehe tools/selftest.sh.

#ifdef HZ_SELFTEST
// Laeuft die Pruefungen und schreibt jede Zeile ins App-Log.
// Rueckgabe: Anzahl der Fehler, 0 = alles wie zugesagt.
int hrv_selftest_run(void);
#endif
