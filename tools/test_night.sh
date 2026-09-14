#!/bin/sh
# Den Weckpfad im Emulator durchspielen.
#
#   sh tools/test_night.sh <Quellordner>
#
# Baut mit -DHZ_TEST_NIGHT (Wecker in einer Minute statt um fünf, Messung 20 s
# statt 180) und -DHZ_FAKE_BEATS (sonst gibt es im Emulator keine Schläge).
# Danach: App verlassen, warten, und nachsehen ob der Wecker sie wirklich
# öffnet, die Messung anläuft und die App sich hinterher wieder schliesst.
#
# Das ist der Teil, der am ehesten still danebengeht: ein Wecker, der nie
# feuert, meldet sich nicht.
export PATH=$HOME/.local/bin:$PATH
SRC="${1:-$HERZINTERVALL_SRC}"
E="--emulator emery"
OUT=$HOME/hz-night
LOG=$HOME/hz-night.log
rm -rf "$OUT"; mkdir -p "$OUT"

shot()  { sleep 1; pebble screenshot $E "$OUT/$1.png" >/dev/null 2>&1; echo "  $(date +%T) $1"; }
click() { pebble emu-button $E click "$1" >/dev/null 2>&1; sleep 1; }

sh "$SRC/tools/sync_herzintervall.sh" "$SRC" night >/dev/null 2>&1 || exit 1
cd $HOME/herzintervall || exit 1

pebble kill >/dev/null 2>&1
sleep 2
pebble wipe >/dev/null 2>&1
pebble install $E >/dev/null 2>&1
sleep 6
pebble logs $E > "$LOG" 2>&1 &
LOGPID=$!
sleep 2
pebble install $E >/dev/null 2>&1
sleep 8

shot 1-start
echo "-- App verlassen, der Wecker soll sie selbst wieder oeffnen --"
click back
sleep 3
shot 2-verlassen

echo "-- warten auf den Wecker (rund eine Minute) --"
sleep 58
shot 3-geweckt
sleep 12
shot 4-waehrend-der-messung
sleep 16
shot 5-nach-der-messung
sleep 6
shot 6-danach

kill $LOGPID 2>/dev/null
pebble kill >/dev/null 2>&1

echo "----- Log -----"
grep -iE "Nachtmessung|Wecker|Messung fertig|Telefon|schliesst" "$LOG" | sed 's/^\[[^]]*\] [^ ]*> //' | head -12
echo "---------------"

D="$HOME/shots"
rm -rf "$D"; mkdir -p "$D"; cp "$OUT"/*.png "$D/" 2>/dev/null
echo "$(ls "$D" | wc -l) Bilder"

# Sauberen Stand wiederherstellen.
sh "$SRC/tools/sync_herzintervall.sh" "$SRC" >/dev/null 2>&1
