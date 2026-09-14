#!/bin/sh
# Die deutschen Texte ansehen.
#
#   sh tools/screens_de.sh <Quellordner>
#
# Der Emulator laeuft immer auf Englisch - i18n_get_system_locale() liefert
# dort "en_US", und es gibt keinen Schalter dafuer. Deshalb wird die Sprachwahl
# in der WSL-KOPIE fest auf Deutsch gestellt. Die Windows-Quelle bleibt
# unberuehrt, und der naechste gewoehnliche Lauf stellt den Stand wieder her.
#
# Geprueft wird damit genau das, was der Emulator sonst verschweigt: ob die
# laengeren deutschen Texte in ihre Felder passen.
export PATH=$HOME/.local/bin:$PATH
SRC="${1:-$HERZINTERVALL_SRC}"
E="--emulator emery"
OUT=$HOME/herzintervall-shots-de
rm -rf "$OUT"; mkdir -p "$OUT"

shot()  { sleep 1; pebble screenshot $E "$OUT/$1.png" >/dev/null 2>&1; echo "  $(date +%T) $1"; }
click() { pebble emu-button $E click "$1" >/dev/null 2>&1; sleep 1; }

sh "$SRC/tools/sync_herzintervall.sh" "$SRC" fake >/dev/null 2>&1 || exit 1
cd $HOME/herzintervall || exit 1
sed -i 's|  return STRINGS_EN;\n}|  return STRINGS_DE;\n}|' src/c/strings.c
# Der Rueckfall steht zweimal in prv_pick_language; nur der letzte ist gemeint.
sed -i '0,/if (strncmp(locale, "de", 2) == 0) return STRINGS_DE;/s||if (1) return STRINGS_DE;|' src/c/strings.c
grep -q 'if (1) return STRINGS_DE' src/c/strings.c || { echo "FEHLER: Sprache nicht umgestellt"; exit 1; }
pebble build 2>&1 | grep -iE 'error|Build failed' && exit 1

pebble kill >/dev/null 2>&1; sleep 2
pebble wipe >/dev/null 2>&1
pebble install $E >/dev/null 2>&1
sleep 7
shot 1-bereit-de
click select
sleep 4
shot 2-messung-de
sleep 56
shot 3-ergebnis-de
pebble kill >/dev/null 2>&1

# Jetzt noch der Fall ohne Sensor, auf Deutsch: gewoehnlicher Bau, aber
# wieder mit fester Sprachwahl.
sh "$SRC/tools/sync_herzintervall.sh" "$SRC" >/dev/null 2>&1
cd $HOME/herzintervall || exit 1
sed -i '0,/if (strncmp(locale, "de", 2) == 0) return STRINGS_DE;/s||if (1) return STRINGS_DE;|' src/c/strings.c
pebble build 2>&1 | grep -iE 'error|Build failed' && exit 1
pebble kill >/dev/null 2>&1; sleep 2
pebble wipe >/dev/null 2>&1
pebble install $E >/dev/null 2>&1
sleep 7
click select
shot 4-kein-hrv-de
pebble kill >/dev/null 2>&1

sh "$SRC/tools/sync_herzintervall.sh" "$SRC" >/dev/null 2>&1
echo "$(ls "$OUT" | wc -l) Bilder in $OUT"
