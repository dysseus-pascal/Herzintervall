#!/bin/sh
# Quellen nach ~/herzintervall spiegeln und bauen (waf vertraegt keine Pfade mit
# Leerzeichen). Aufruf: sync_herzintervall.sh [<Quellordner>] [<schalter>]
# Ohne Argument wird $HERZINTERVALL_SRC verwendet.
#
# Schalter als zweites Argument:
#   selftest   -DHZ_SELFTEST    Rechnung beim Start pruefen, Ergebnis ins Log
#   fake       -DHZ_FAKE_BEATS  erfundene Schlagintervalle, damit sich der
#                               Ablauf im Emulator durchspielen laesst
#   both       beides
#   night      -DHZ_TEST_NIGHT  Wecker in 1 Minute statt um 5 Uhr, Messung 20 s
#              (zusammen mit fake, sonst gibt es im Emulator keine Schlaege)
# Alle nur in der WSL-KOPIE; die Windows-Quelle bleibt unberuehrt.
export PATH=$HOME/.local/bin:$PATH
SRC="${1:-$HERZINTERVALL_SRC}"
MODE="$2"
[ -d "$SRC" ] || { echo "Quellordner fehlt: '$SRC'"; exit 1; }
DST=$HOME/herzintervall
mkdir -p "$DST"
# waf erzeugt message_keys.auto.h aus package.json und merkt eine Aenderung
# daran nicht - deshalb aufraeumen, sobald sich package.json unterscheidet.
if ! cmp -s "$SRC/package.json" "$DST/package.json"; then
  (cd "$DST" && pebble clean >/dev/null 2>&1)
fi
rm -rf "$DST/src" "$DST/build"
cp -r "$SRC/src" "$DST/"
cp "$SRC/package.json" "$SRC/wscript" "$DST/"
if [ -d "$SRC/resources" ]; then
  rm -rf "$DST/resources"
  cp -r "$SRC/resources" "$DST/"
fi
cd "$DST" || exit 1

FLAGS=""
case "$MODE" in
  selftest) FLAGS="-DHZ_SELFTEST" ;;
  fake)     FLAGS="-DHZ_FAKE_BEATS" ;;
  both)     FLAGS="-DHZ_SELFTEST -DHZ_FAKE_BEATS" ;;
  night)    FLAGS="-DHZ_FAKE_BEATS -DHZ_TEST_NIGHT" ;;
esac
if [ -n "$FLAGS" ]; then
  # Anker ist eine Zeile, die es NUR in build() gibt. ctx.load('pebble_sdk')
  # steht auch in options() und configure() - dort hat ctx kein env, und der
  # Bau bricht mit "OptionsContext object has no attribute env" ab.
  ANCHOR="    build_worker = os.path.exists('worker_src')"
  {
    echo "    for _e in ctx.all_envs.values():"
    echo "        _e.append_value('CFLAGS', \"$FLAGS\".split())"
    echo ""
    echo "$ANCHOR"
  } > /tmp/hz_patch.txt
  awk -v anchor="$ANCHOR" 'BEGIN{while((getline l < "/tmp/hz_patch.txt")>0) p=p l "\n"}
       $0==anchor{printf "%s", p; next} {print}' wscript > /tmp/hz_wscript
  mv /tmp/hz_wscript wscript
  grep -q "append_value('CFLAGS'" wscript || { echo "FEHLER: Schalter nicht eingesetzt"; exit 1; }
  echo "Pruefbau mit $FLAGS"
fi

echo "Dateien in src/c: $(ls src/c | wc -l)"
pebble build 2>&1 | grep -iE 'error|warning: \.\./src|APP MEMORY|footprint in RAM|finished successfully|Build failed|Traceback'
