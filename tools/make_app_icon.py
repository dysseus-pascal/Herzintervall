#!/usr/bin/env python3
"""App-Symbol: die drei Buchstaben HRV.

Aufruf: make_app_icon.py <zielordner>          -> system_icon.png (25x25)
        make_app_icon.py --store <zielordner>  -> icon-144.png, icon-48.png

Kein Herz. Ein Herz sagt "Puls", nicht "Herzratenvariabilitaet" - und Puls
zeigt jede Uhr von Haus aus. HRV benennt genau das, was diese App misst und
was sie von der Systemfunktion unterscheidet.

Die Buchstaben sind aus Balken und Strecken gebaut, nicht gesetzt: eine
Systemschrift steht auf der Uhr erst zur Laufzeit zur Verfuegung, ein Symbol
muss aber schon im Bau fertig sein.

MASSSTAB IST DAS SYSTEMSYMBOL. Die Uhr-Kachel von "Watchfaces" im Starter wurde
Punkt fuer Punkt nachgemessen: 24 von 25 Punkten hoch, Linien 2 bis 3 Punkte
stark, rund 180 schwarze Punkte. Danach richten sich Groesse und Strichstaerke
hier - eine duennere Linie sieht daneben aus wie ein Versehen.

NUR LINIEN, KEINE FLAECHE, und keine ~bw-Fassung. Der Starter zeichnet Symbole
einfarbig: eine farbige Flaeche kam dort als grauer Fleck heraus (im Emulator
nachgemessen - Rot 255,0,0 wurde zu Grau 171,171,171). Eine schwarze Linie ist
auf jeder Uhr dieselbe Datei.

Drei Buchstaben auf 25 Punkten sind eng. Deshalb 7 Punkte je Buchstabe, ein
Punkt Abstand, und Versalhoehe 17 - das fuellt den Kasten so weit wie das
Vorbild, ohne dass die Innenraeume zulaufen.

DER STORE NIMMT NICHTS AUS DER .pbw. Im Entwicklerportal liegen zwei eigene
Bilder, `icon_large` und `icon_small`; angefordert werden sie in festen Massen
(gross 80 und 144, klein 28 und 48), jeweils mit `exact` in der Adresse, also
erzwungen statt eingepasst - etwas Nicht-Quadratisches kommt verzogen zurueck.
Darum hier eine gefuellte Kachel: das grosse Symbol legt der Store fuer sein
Teilen-Bild durch eine abgerundete Maske, und ueber einer durchsichtigen
Strichzeichnung taete die nichts.

BEI 28 PUNKTEN SIND DREI BUCHSTABEN AM RAND DES LESBAREN. Das ist der Preis
eines Wortzeichens und keine Schwaeche der Kachel: das kleine Symbol steht im
Store neben dem Titel, der daneben ohnehin ausgeschrieben ist.

DIE FORM STEHT NUR EINMAL DA. Alle Masse gelten auf einem Raster von 25
Punkten und werden mit s hochgerechnet; mit s = 1 kommt Punkt fuer Punkt das
alte Bild heraus.
"""
import os
import struct
import sys
import zlib

RASTER = 25                      # Bezugsraster, auf dem alle Masse gelten
SS = 4                           # Ueberabtastung je Achse
LINE = 2                         # Strichstaerke in Punkten, wie beim Vorbild

# Drei Buchstaben sind dichter als eine Umrisszeichnung: mit Versalhoehe 20 kam
# HRV auf 256 schwarze Punkte gegen 180 beim Vorbild. Eine Spur kleiner bringt
# es in dieselbe Gegend, ohne dass die Innenraeume zulaufen.
TOP, BOT = 3.0, 21.0             # Versalhoehe
LW = LINE / 2.0 - 0.15           # halbe Strichstaerke; die Schraegen laufen
                                 # sonst breiter als die geraden Balken
ADV = 8.0                        # Vorschub je Buchstabe
X0 = 0.5                         # linker Rand des ersten Buchstabens

# Store-Kachel. Der Wert stammt aus src/c/theme.h (HZ_COLOR_SIDEBAR),
# nachgeschlagen in gcolor_definitions.h des SDK - nicht aus dem Gedaechtnis.
GRUND = (0x55, 0x00, 0x55)       # GColorImperialPurple
STRICH = (0xFF, 0xFF, 0xFF)      # weiss
# Etwas mehr als bei den Geschwistern: drei Buchstaben brauchen die Breite,
# sonst stehen sie als Faden in der Mitte der Kachel.
FUELL = 0.78
STORE_GROESSEN = (144, 48)


def png(path, w, h, rows):
    """Minimaler PNG-Schreiber, 8 Bit RGBA, ohne Fremdbibliothek."""
    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xffffffff)
    raw = b"".join(b"\x00" + bytes(r) for r in rows)
    out = b"\x89PNG\r\n\x1a\n"
    out += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    out += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(out)


def raster(test, n):
    """Vierfach ueberabtasten, bei halber Deckung schneiden. Harte Kanten."""
    grid = []
    for py in range(n):
        row = []
        for px in range(n):
            hits = 0
            for sy in range(SS):
                for sx in range(SS):
                    if test(px + (sx + 0.5) / SS, py + (sy + 0.5) / SS):
                        hits += 1
            row.append(hits * 2 >= SS * SS)
        grid.append(row)
    return grid


def seg(x, y, ax, ay, bx, by, r):
    """Liegt der Punkt hoechstens r von der Strecke a-b entfernt?"""
    dx, dy = bx - ax, by - ay
    L2 = dx * dx + dy * dy
    t = 0.0 if L2 == 0 else ((x - ax) * dx + (y - ay) * dy) / L2
    t = max(0.0, min(1.0, t))
    ex, ey = x - (ax + t * dx), y - (ay + t * dy)
    return ex * ex + ey * ey <= r * r


def bar(x, y, x0, y0, x1, y1):
    """Ein gerades Balkenstueck, Ecken eingeschlossen."""
    return x0 <= x <= x1 and y0 <= y <= y1


def pruefer(s):
    """Der Formtest, auf den Massstab s gebracht.

    Auch die Rundungszugaben (LINE - 1, das -1 in der Querbalkenkante) werden
    mitskaliert. Sie sind Zugaben auf ein Raster, kein fester Abstand - wer sie
    stehen liesse, bekaeme bei 144 Punkten einen Haarriss statt einer Kante.
    """
    top, bot = TOP * s, BOT * s
    lw, adv, x0 = LW * s, ADV * s, X0 * s
    e = (LINE - 1) * s               # Balkenbreite, Kante eingeschlossen
    eins = 1.0 * s

    def glyph_h(x, y, ox):
        """H: zwei Stiele und ein Querbalken."""
        mid = (top + bot) / 2.0
        return (bar(x, y, ox, top, ox + e, bot)
                or bar(x, y, ox + 5 * s, top, ox + 5 * s + e, bot)
                or bar(x, y, ox, mid - lw, ox + 5 * s + e, mid + lw - eins))

    def glyph_r(x, y, ox):
        """R: Stiel, Kopf, Querbalken, Bein."""
        waist = top + (bot - top) * 0.45
        return (bar(x, y, ox, top, ox + e, bot)
                or bar(x, y, ox, top, ox + 5 * s + e, top + e)
                or bar(x, y, ox + 5 * s, top, ox + 5 * s + e, waist)
                or bar(x, y, ox, waist - lw, ox + 5 * s + e, waist + lw - eins)
                or seg(x, y, ox + 3 * s, waist, ox + 5 * s + lw, bot, lw))

    def glyph_v(x, y, ox):
        """V: zwei Schraegen, die sich unten treffen."""
        return (seg(x, y, ox + lw, top, ox + 3 * s + lw, bot, lw)
                or seg(x, y, ox + 6 * s + lw, top, ox + 3 * s + lw, bot, lw))

    def inside(x, y):
        return (glyph_h(x, y, x0)
                or glyph_r(x, y, x0 + adv)
                or glyph_v(x, y, x0 + 2 * adv))
    return inside


def schreibe_uhr(dest):
    n = RASTER
    grid = raster(pruefer(1.0), n)
    rows = []
    for y in range(n):
        r = []
        for x in range(n):
            r += [0, 0, 0, 255] if grid[y][x] else [0, 0, 0, 0]
        rows.append(r)
    png(os.path.join(dest, "system_icon.png"), n, n, rows)
    old = os.path.join(dest, "system_icon~bw.png")
    if os.path.exists(old):
        os.remove(old)
        print("system_icon~bw.png entfernt - die Linie gilt fuer alle Uhren")
    punkte = sum(1 for r in grid for v in r if v)
    ys = [y for y in range(n) if any(grid[y])]
    print("system_icon.png: %d Punkte schwarz, %d hoch (Vorbild: 180 / 24)"
          % (punkte, (ys[-1] - ys[0] + 1) if ys else 0))


def schreibe_store(dest):
    for gross in STORE_GROESSEN:
        innen = int(round(gross * FUELL))
        grid = raster(pruefer(innen / float(RASTER)), innen)
        rand = (gross - innen) // 2
        rows = []
        for y in range(gross):
            r = []
            for x in range(gross):
                iy, ix = y - rand, x - rand
                treffer = 0 <= iy < innen and 0 <= ix < innen and grid[iy][ix]
                farbe = STRICH if treffer else GRUND
                r += [farbe[0], farbe[1], farbe[2], 255]
            rows.append(r)
        name = "icon-%d.png" % gross
        png(os.path.join(dest, name), gross, gross, rows)
        print("%s: Kachel %s, HRV weiss" % (name, "#%02X%02X%02X" % GRUND))


def main():
    args = sys.argv[1:]
    store = "--store" in args
    if store:
        args.remove("--store")
    dest = args[0] if args else ("store" if store else "resources/images")
    os.makedirs(dest, exist_ok=True)
    if store:
        schreibe_store(dest)
    else:
        schreibe_uhr(dest)


if __name__ == "__main__":
    main()
