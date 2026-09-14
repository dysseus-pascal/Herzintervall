#pragma once
#include <pebble.h>

// Farbschema violett/weiss im Stil der Pebble-Timeline, wie in Drinktervall
// und Flynformer: weisser Grund, schwarze Schrift, dunkle Seitenleiste rechts
// mit den Tastenhinweisen. Alle Farben nur hier aendern.
//
// Warum Violett: Drinktervall ist blau, Flynformer orange. Eine dritte,
// deutlich andere Familie haelt die Apps im Starter auseinander. Rot laege
// beim Herzen naeher, saesse aber zu dicht an Flynformer.
//
// Der Schirm der Uhr LEUCHTET NICHT, er spiegelt. Helle Flaechen lesen sich
// draussen besser als dunkle - deshalb Weiss als Grund und die Farbe nur in
// Leiste und Balken.
#define HZ_COLOR_BG          GColorWhite
#define HZ_COLOR_TEXT        GColorBlack

#define HZ_SIDEBAR_W         PBL_IF_ROUND_ELSE(51, (PBL_DISPLAY_WIDTH >= 180 ? 34 : 30))
#define HZ_COLOR_SIDEBAR     PBL_IF_COLOR_ELSE(GColorImperialPurple, GColorBlack)
#define HZ_COLOR_ON_SIDEBAR  GColorWhite

// Gefuellte Segmente des Fortschrittsbalkens; die leeren bleiben hellgrau.
#define HZ_COLOR_BAR         PBL_IF_COLOR_ELSE(GColorLavenderIndigo, GColorBlack)
#define HZ_COLOR_BAR_EMPTY   PBL_IF_COLOR_ELSE(GColorLightGray, GColorLightGray)

// Grosse Zahl (RMSSD, Restsekunden) und die kleine Nebenzeile darunter.
#define HZ_COLOR_BIG         PBL_IF_COLOR_ELSE(GColorImperialPurple, GColorBlack)
#define HZ_COLOR_DIM         PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack)

// Herz oben in der Seitenleiste
#define HZ_HEART_DX          PBL_IF_ROUND_ELSE(9, 0)   // rund: sichtbarer Teil der Leiste
#define HZ_HEART_Y           PBL_IF_ROUND_ELSE(58, 22)
#define HZ_HEART_W           20

#define HZ_MARGIN            PBL_IF_ROUND_ELSE(38, 9)
