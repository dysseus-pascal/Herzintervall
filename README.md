# Herzintervall

Herzratenvariabilität auf der Uhr messen: eine Minute ruhig sitzen, am Ende
steht der **RMSSD** in Millisekunden da.

Die Oberfläche folgt der **Sprache der Uhr** (Deutsch und Englisch, Englisch als
Rückfall) und dem Timeline-Look der Schwesterapps: weisser Grund, schwarze
Schrift, dunkle Seitenleiste rechts.

> **Nur Pebble Time 2.** Keine andere Uhr liefert Schlagintervalle — und der
> Emulator auch nicht. Mehr dazu unter [Was die Uhr hergibt](#was-die-uhr-hergibt).

## Screenshots

| | | |
|---|---|---|
| ![Bereit](screenshots/emery/01-bereit.png) | ![Messung](screenshots/emery/02-messung.png) | ![Messung](screenshots/emery/03-messung-haelfte.png) |
| bereit | Messung beginnt | Messung, Balken halb |
| ![Ergebnis](screenshots/emery/04-ergebnis.png) | ![Deutsch](screenshots/emery/06-ergebnis-de.png) | ![Kein HRV](screenshots/emery/05-kein-hrv.png) |
| Ergebnis | dasselbe auf Deutsch | Uhr ohne HRV |

## Bedienung

Ein Bildschirm, eine Taste.

| Taste | Aktion |
|---|---|
| Mitte | Messung starten, nochmal drücken bricht ab |
| Zurück | App verlassen |

Vier Zustände: **bereit** (der letzte Wert steht da), **Messung läuft**
(Restsekunden gross, darunter gezählte und verworfene Schläge, unten ein
Balken), **Ergebnis** (RMSSD gross, mittlerer Puls und Anzahl darunter) und
**kein HRV**, wenn die Uhr keine Intervalle liefert.

Der letzte Wert überlebt das Beenden und steht auch im App-Glance des Starters.

## Was gemessen wird

**RMSSD** — die Wurzel aus dem Mittel der quadrierten Unterschiede
aufeinanderfolgender Schlagintervalle. Von den gängigen HRV-Kennzahlen ist sie
die, die schon aus einer Minute etwas Belastbares macht.

Gerechnet wird in ganzen Zahlen, mit eigener Quadratwurzel: die Uhr hat keine
Fliesskomma-Einheit.

### Warum gefiltert wird

Die Uhr liefert jedes Intervall einzeln, aber **ohne seine Güte**. Der
Health-Handler bekommt nur den Ereignistyp, und die Struktur mit dem
Konfidenzwert ist in der Firmware als `@internal` markiert — für Apps
unsichtbar. Ein verschluckter oder doppelt gezählter Herzschlag ist dem Wert
also nicht anzusehen, und genau darauf reagiert RMSSD heftiger als auf alles
andere.

Deshalb sortiert `src/c/hrv_math.c` statistisch aus:

1. **Fenster 300–2000 ms** (200 bis 30 Schläge/min). Alles ausserhalb ist kein
   Herzschlag, sondern ein Messfehler.
2. **Höchstens 20 % Sprung** zum vorigen angenommenen Intervall. Echte
   Schlag-zu-Schlag-Schwankung bleibt darunter, ein ausgelassener Schlag
   (fast doppelte Länge) liegt darüber.
3. **Differenzen nur über ununterbrochene Paare.** Wurde dazwischen etwas
   verworfen, fehlt ein Schlag — eine Differenz über die Lücke hinweg wäre
   erfunden.
4. **Nach drei verworfenen in Folge wird neu angesetzt.** Ohne das bliebe der
   Filter hängen, sobald der Puls schnell steigt: er vergliche ewig gegen einen
   alten Wert und verwürfe jeden neuen.

Unter fünf brauchbaren Differenzen wird kein Wert angezeigt, sondern «zu wenig
saubere Schläge».

## Was die Uhr hergibt

Seit Firmware 4.32 gibt es die HRV-API, und die SDK 4.33.1 hat sie bereits.
Drei Dinge sind dabei wichtig:

- **Nur auf der Pebble Time 2.** `CONFIG_HRM_HRV` steht in PebbleOS allein im
  Board `obelix`. `asterix` (Pebble 2 Duo) und `getafix` (Pebble Round 2) haben
  gar keinen Herzsensor, und `qemu_emery` hat `CONFIG_HRM` **ohne** HRV.
- **Die Header liegen für alle Plattformen bereit.** Eine HRV-App baut deshalb
  auch für flint und gabbro anstandslos durch und tut dort zur Laufzeit nichts.
  `health_service_set_hrv_sample_period()` muss auf `false` geprüft werden —
  sonst sucht man den Fehler an der falschen Stelle. Genau das führt hier zum
  Bildschirm «kein HRV».
- **Nichts wird gespeichert.** HRV taucht in der Health-Datenbank nicht auf. Es
  wird nur gesammelt, solange diese App läuft und den Sensor angefordert hat.
  Beim Beenden gibt sie ihn wieder frei — sonst liefe er weiter und kostete
  Batterie.

Ausserhalb des Handgelenks meldet die Firmware ein Intervall 0, also dasselbe
wie «noch nichts gemessen». Beides ist von aussen nicht unterscheidbar.

## Bauen

Pebble waf verträgt keine Pfade mit Leerzeichen, deshalb wird in WSL unter
`~/herzintervall` gebaut:

```sh
tools/sync_herzintervall.sh <Quellordner>   # spiegeln + bauen
pebble install --emulator emery
pebble install --phone <IP>                 # Developer Connection der Pebble-App
```

Die App zielt nur auf **emery** (Pebble Time 2). Andere Plattformen im
Appstore anzubieten hiesse, eine App auszuliefern, die dort nichts tun kann.

## Prüfen

```sh
sh tools/selftest.sh <Quellordner>    # die Rechnung, auf der Zielarchitektur
sh tools/screens.sh <Quellordner>     # alle Bildschirme fotografieren
sh tools/screens_de.sh <Quellordner>  # dieselben auf Deutsch
node tools/strings_check.js src/c/strings_table.h
```

**Warum der Selbsttest im Emulator läuft und nicht hier:** auf dem Baurechner
steht kein C-Compiler — kein gcc, und ohne Passwort kein `apt`. Nur der
ARM-Compiler der SDK ist da. Ein Test, der nie läuft, ist keiner; also läuft er
dort, wo ohnehin übersetzt wird. Das prüft nebenbei mehr als ein Lauf auf
x86-64, weil Typbreiten und Ausrichtung dann nachweislich auf 32-Bit-ARM
stimmen.

Die Erwartungswerte sind **vorher unabhängig bestimmt** (Lehrbuchformel in
Fliesskomma) und als Konstanten eingetragen. Ein Test, der seine Erwartung aus
demselben Code zieht, den er prüft, bestätigt nur sich selbst.

`screens.sh` läuft zweimal: einmal gewöhnlich — der Emulator hat kein HRV, das
zeigt also den echten Fehlerweg ungetrickst — und einmal mit
`-DHZ_FAKE_BEATS`, damit sich Messung und Ergebnis überhaupt ansehen lassen.
Gezeichnet und gerechnet wird dabei dasselbe wie im Betrieb; nur die Herkunft
der Zahlen ist gefälscht.

## Was hier NICHT geprüft ist

Der Sensor. Es gibt auf diesem Rechner keinen Weg dazu — kein Emulator bildet
ihn nach. Ungeprüft bleibt damit alles, was erst am Handgelenk entsteht:

- ob `health_service_set_hrv_sample_period(1)` auf der echten Uhr angenommen wird
- wie dicht die Intervalle tatsächlich eintreffen
- wie viele der Filter bei echtem PPG-Rauschen verwirft
- ob ein Ereignis verlorengeht, wenn zwei Intervalle dicht aufeinander folgen
  (`peek` liefert immer nur das letzte — mit der öffentlichen API ist das nicht
  zu umgehen)

Das muss der erste Lauf auf der Uhr zeigen.

## Herkunft

Eigenentwicklung. Der Timeline-Look und die Bausteine (Seitenleiste,
Segmentbalken, Textmaschinerie) stammen aus den Schwesterapps Drinktervall und
Flynformer.

## Lizenz

Gemeinfrei, [CC0 1.0](LICENSE). Kopieren, ändern, verkaufen, einbauen — ohne
Bedingung, ohne Namensnennung, ohne Rückfrage.

CC0 statt der Unlicense, weil das Schweizer Urheberrecht einen Verzicht gar
nicht kennt; CC0 trägt für genau diesen Fall eine Ersatzlizenz in sich, die
dasselbe erlaubt.
