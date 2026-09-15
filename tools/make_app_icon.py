#!/usr/bin/env python3
"""App-Symbol: die drei Buchstaben HRV (25x25).

Aufruf: make_app_icon.py <zielordner>
Erzeugt system_icon.png - schwarze Linien auf durchsichtigem Grund.

Kein Herz mehr. Ein Herz sagt "Puls", nicht "Herzratenvariabilitaet" - und
Puls zeigt jede Uhr von Haus aus. HRV benennt genau das, was diese App misst
und was sie von der Systemfunktion unterscheidet.

Die Buchstaben sind aus Balken und Strecken gebaut, nicht gesetzt: eine
Systemschrift steht auf der Uhr erst zur Laufzeit zur Verfuegung, ein Symbol
muss aber schon im Bau fertig sein.

MASSSTAB IST DAS SYSTEMSYMBOL. Die Uhr-Kachel von "Watchfeces" im Starter wurde
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
"""
import os
import struct
import sys
import zlib

W = H = 25
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


def raster(test):
    """Vierfach ueberabtasten, bei halber Deckung schneiden. Harte Kanten."""
    grid = []
    for py in range(H):
        row = []
        for px in range(W):
            hits = 0
            for sy in range(SS):
                for sx in range(SS):
                    if test(px + (sx + 0.5) / SS, py + (sy + 0.5) / SS):
                        hits += 1
            row.append(hits * 2 >= SS * SS)
        grid.append(row)
    return grid


def write(dest, grid):
    rows = []
    for y in range(H):
        r = []
        for x in range(W):
            r += [0, 0, 0, 255] if grid[y][x] else [0, 0, 0, 0]
        rows.append(r)
    png(os.path.join(dest, "system_icon.png"), W, H, rows)
    old = os.path.join(dest, "system_icon~bw.png")
    if os.path.exists(old):
        os.remove(old)
        print("system_icon~bw.png entfernt - die Linie gilt fuer alle Uhren")
    n = sum(1 for r in grid for v in r if v)
    ys = [y for y in range(H) if any(grid[y])]
    print("system_icon.png: %d Punkte schwarz, %d hoch (Vorbild: 180 / 24)"
          % (n, (ys[-1] - ys[0] + 1) if ys else 0))


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


def glyph_h(x, y, ox):
    """H: zwei Stiele und ein Querbalken."""
    mid = (TOP + BOT) / 2.0
    return (bar(x, y, ox, TOP, ox + LINE - 1, BOT)
            or bar(x, y, ox + 5, TOP, ox + 5 + LINE - 1, BOT)
            or bar(x, y, ox, mid - LW, ox + 5 + LINE - 1, mid + LW - 1))


def glyph_r(x, y, ox):
    """R: Stiel, Kopf, Querbalken, Bein."""
    waist = TOP + (BOT - TOP) * 0.45
    return (bar(x, y, ox, TOP, ox + LINE - 1, BOT)
            or bar(x, y, ox, TOP, ox + 5 + LINE - 1, TOP + LINE - 1)
            or bar(x, y, ox + 5, TOP, ox + 5 + LINE - 1, waist)
            or bar(x, y, ox, waist - LW, ox + 5 + LINE - 1, waist + LW - 1)
            or seg(x, y, ox + 3, waist, ox + 5 + LW, BOT, LW))


def glyph_v(x, y, ox):
    """V: zwei Schraegen, die sich unten treffen."""
    return (seg(x, y, ox + LW, TOP, ox + 3 + LW, BOT, LW)
            or seg(x, y, ox + 6 + LW, TOP, ox + 3 + LW, BOT, LW))


def inside(x, y):
    return (glyph_h(x, y, X0)
            or glyph_r(x, y, X0 + ADV)
            or glyph_v(x, y, X0 + 2 * ADV))


def main():
    dest = sys.argv[1] if len(sys.argv) > 1 else "resources/images"
    os.makedirs(dest, exist_ok=True)
    write(dest, raster(inside))


if __name__ == "__main__":
    main()
