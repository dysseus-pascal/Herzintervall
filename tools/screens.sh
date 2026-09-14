#!/bin/sh
# Bildschirme im Emulator fotografieren.
#
#   sh tools/screens.sh <Quellordner>
#
# Zwei Durchlaeufe, weil der Emulator KEINEN HRV-Sensor hat (CONFIG_HRM_HRV
# steht nur im Board obelix, nicht in qemu_emery):
#
#   1. gewoehnlicher Bau  -> zeigt genau das, was eine Uhr ohne HRV zeigt.
#      Das ist kein Notbehelf, sondern der echte Fehlerweg, ungetrickst.
#   2. Bau mit -DHZ_FAKE_BEATS -> erfundene Intervalle, damit sich Messung und
#      Ergebnis ueberhaupt ansehen lassen. Gezeichnet wird dasselbe wie im
#      Betrieb; nur die Herkunft der Zahlen ist gefaelscht.
#
# Die Bilder landen unter $HOME/herzintervall-shots (NICHT /tmp - das wird
# zwischen WSL-Sitzungen geleert).
export PATH=$HOME/.local/bin:$PATH
SRC="${1:-$HERZINTERVALL_SRC}"
E="--emulator emery"
OUT=$HOME/herzintervall-shots
rm -rf "$OUT"; mkdir -p "$OUT"

shot()  { sleep 1; pebble screenshot $E "$OUT/$1.png" >/dev/null 2>&1; echo "  $(date +%T) $1"; }
click() { pebble emu-button $E click "$1" >/dev/null 2>&1; sleep 1; }

boot() {
  pebble kill >/dev/null 2>&1
  sleep 2
  pebble wipe >/dev/null 2>&1
  pebble install $E >/dev/null 2>&1
  sleep 7
}

echo "== 1. ohne Sensor (gewoehnlicher Bau) =="
sh "$SRC/tools/sync_herzintervall.sh" "$SRC" >/dev/null 2>&1 || exit 1
cd $HOME/herzintervall || exit 1
boot
shot 1-bereit
click select
shot 2-kein-hrv

echo "== 2. mit erfundenen Schlaegen =="
sh "$SRC/tools/sync_herzintervall.sh" "$SRC" fake >/dev/null 2>&1 || exit 1
cd $HOME/herzintervall || exit 1
boot
shot 3-bereit
click select
sleep 4
shot 4-messung
sleep 25
shot 5-messung-haelfte
sleep 34
shot 6-ergebnis
click select
sleep 3
shot 7-zweite-messung
pebble kill >/dev/null 2>&1

# Den sauberen Stand wiederherstellen, damit kein Pruefbau liegenbleibt.
sh "$SRC/tools/sync_herzintervall.sh" "$SRC" >/dev/null 2>&1
echo "$(ls "$OUT" | wc -l) Bilder in $OUT"
