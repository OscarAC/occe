#include "colors.h"
#include <stdio.h>
#include <string.h>

/* Default color scheme (similar to popular editors) */
ColorPair default_colors[HL_MAX] = {
    [HL_NORMAL]      = {COLOR_DEFAULT, COLOR_DEFAULT, ATTR_NORMAL},
    [HL_KEYWORD]     = {COLOR_MAGENTA, COLOR_DEFAULT, ATTR_BOLD},
    [HL_TYPE]        = {COLOR_CYAN, COLOR_DEFAULT, ATTR_NORMAL},
    [HL_STRING]      = {COLOR_YELLOW, COLOR_DEFAULT, ATTR_NORMAL},
    [HL_NUMBER]      = {COLOR_BRIGHT_CYAN, COLOR_DEFAULT, ATTR_NORMAL},
    [HL_COMMENT]     = {COLOR_BRIGHT_BLACK, COLOR_DEFAULT, ATTR_ITALIC},
    [HL_OPERATOR]    = {COLOR_WHITE, COLOR_DEFAULT, ATTR_NORMAL},
    [HL_FUNCTION]    = {COLOR_BLUE, COLOR_DEFAULT, ATTR_BOLD},
    [HL_VARIABLE]    = {COLOR_WHITE, COLOR_DEFAULT, ATTR_NORMAL},
    [HL_CONSTANT]    = {COLOR_BRIGHT_MAGENTA, COLOR_DEFAULT, ATTR_NORMAL},
    [HL_PREPROCESSOR] = {COLOR_BRIGHT_RED, COLOR_DEFAULT, ATTR_NORMAL},
    [HL_ERROR]       = {COLOR_RED, COLOR_DEFAULT, ATTR_REVERSE}
};

void colors_init(void) {
    /* Colors are already initialized statically */
}

ColorPair colors_get(HighlightType type) {
    if (type >= 0 && type < HL_MAX) {
        return default_colors[type];
    }
    return default_colors[HL_NORMAL];
}

void colors_set(HighlightType type, Color fg, Color bg, uint8_t attrs) {
    if (type >= 0 && type < HL_MAX) {
        default_colors[type].fg = fg;
        default_colors[type].bg = bg;
        default_colors[type].attrs = attrs;
    }
}

void colors_to_ansi(ColorPair cp, char *buf, size_t bufsize) {
    int pos = 0;

    /* Start escape sequence */
    pos += snprintf(buf + pos, bufsize - pos, "\x1b[");

    /* Add attributes */
    if (cp.attrs & ATTR_BOLD) {
        pos += snprintf(buf + pos, bufsize - pos, "1;");
    }
    if (cp.attrs & ATTR_DIM) {
        pos += snprintf(buf + pos, bufsize - pos, "2;");
    }
    if (cp.attrs & ATTR_ITALIC) {
        pos += snprintf(buf + pos, bufsize - pos, "3;");
    }
    if (cp.attrs & ATTR_UNDERLINE) {
        pos += snprintf(buf + pos, bufsize - pos, "4;");
    }
    if (cp.attrs & ATTR_REVERSE) {
        pos += snprintf(buf + pos, bufsize - pos, "7;");
    }

    /* Foreground color */
    if (cp.fg != COLOR_DEFAULT) {
        if (cp.fg >= COLOR_BRIGHT_BLACK) {
            /* Bright colors (90-97) */
            pos += snprintf(buf + pos, bufsize - pos, "%d;", 90 + (cp.fg - COLOR_BRIGHT_BLACK));
        } else {
            /* Normal colors (30-37) */
            pos += snprintf(buf + pos, bufsize - pos, "%d;", 30 + cp.fg);
        }
    }

    /* Background color */
    if (cp.bg != COLOR_DEFAULT) {
        if (cp.bg >= COLOR_BRIGHT_BLACK) {
            /* Bright backgrounds (100-107) */
            pos += snprintf(buf + pos, bufsize - pos, "%d;", 100 + (cp.bg - COLOR_BRIGHT_BLACK));
        } else {
            /* Normal backgrounds (40-47) */
            pos += snprintf(buf + pos, bufsize - pos, "%d;", 40 + cp.bg);
        }
    }

    /* Remove trailing semicolon if present */
    if (pos > 2 && buf[pos - 1] == ';') {
        pos--;
    }

    /* Close escape sequence */
    snprintf(buf + pos, bufsize - pos, "m");
}
