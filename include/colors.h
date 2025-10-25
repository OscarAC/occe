#ifndef COLORS_H
#define COLORS_H

#include <stddef.h>
#include <stdint.h>

/* Terminal color codes (ANSI) */
typedef enum {
    COLOR_BLACK = 0,
    COLOR_RED = 1,
    COLOR_GREEN = 2,
    COLOR_YELLOW = 3,
    COLOR_BLUE = 4,
    COLOR_MAGENTA = 5,
    COLOR_CYAN = 6,
    COLOR_WHITE = 7,
    COLOR_BRIGHT_BLACK = 8,
    COLOR_BRIGHT_RED = 9,
    COLOR_BRIGHT_GREEN = 10,
    COLOR_BRIGHT_YELLOW = 11,
    COLOR_BRIGHT_BLUE = 12,
    COLOR_BRIGHT_MAGENTA = 13,
    COLOR_BRIGHT_CYAN = 14,
    COLOR_BRIGHT_WHITE = 15,
    COLOR_DEFAULT = -1
} Color;

/* Text attributes */
typedef enum {
    ATTR_NORMAL = 0,
    ATTR_BOLD = 1,
    ATTR_DIM = 2,
    ATTR_ITALIC = 3,
    ATTR_UNDERLINE = 4,
    ATTR_BLINK = 5,
    ATTR_REVERSE = 7
} TextAttr;

/* Color pair for foreground/background */
typedef struct {
    Color fg;
    Color bg;
    uint8_t attrs;  /* Bitmask of TextAttr */
} ColorPair;

/* Predefined color schemes for syntax highlighting */
typedef enum {
    HL_NORMAL = 0,      /* Normal text */
    HL_KEYWORD,         /* Language keywords */
    HL_TYPE,            /* Type names */
    HL_STRING,          /* String literals */
    HL_NUMBER,          /* Number literals */
    HL_COMMENT,         /* Comments */
    HL_OPERATOR,        /* Operators */
    HL_FUNCTION,        /* Function names */
    HL_VARIABLE,        /* Variables */
    HL_CONSTANT,        /* Constants */
    HL_PREPROCESSOR,    /* Preprocessor directives */
    HL_ERROR,           /* Errors */
    HL_MAX
} HighlightType;

/* Default color scheme */
extern ColorPair default_colors[HL_MAX];

/* Initialize color system */
void colors_init(void);

/* Get color pair for highlight type */
ColorPair colors_get(HighlightType type);

/* Set custom colors */
void colors_set(HighlightType type, Color fg, Color bg, uint8_t attrs);

/* Generate ANSI escape sequence for color pair */
void colors_to_ansi(ColorPair cp, char *buf, size_t bufsize);

#endif /* COLORS_H */
