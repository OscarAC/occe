#ifndef THEME_H
#define THEME_H

#include "colors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Extended color type supporting TrueColor (24-bit RGB) */
typedef struct {
    bool is_rgb;           /* true = RGB mode, false = ANSI mode */
    union {
        Color ansi;        /* ANSI color code (0-15 or -1 for default) */
        struct {
            uint8_t r;
            uint8_t g;
            uint8_t b;
        } rgb;             /* RGB values (0-255 each) */
    };
} ThemeColor;

/* Extended color pair with TrueColor support */
typedef struct {
    ThemeColor fg;
    ThemeColor bg;
    uint8_t attrs;         /* Bitmask of TextAttr */
} ThemeColorPair;

/* Theme definition */
typedef struct Theme {
    char *name;                          /* Theme name */
    char *author;                        /* Theme author */
    char *description;                   /* Theme description */
    bool dark_mode;                      /* Is this a dark theme? */

    /* Color pairs for all highlight types */
    ThemeColorPair colors[HL_MAX];

    /* UI colors (editor chrome) */
    ThemeColor background;               /* Editor background */
    ThemeColor foreground;               /* Default text color */
    ThemeColor selection_bg;             /* Selection background */
    ThemeColor selection_fg;             /* Selection foreground */
    ThemeColor line_number;              /* Line number gutter */
    ThemeColor current_line_bg;          /* Current line highlight */
    ThemeColor cursor;                   /* Cursor color */
    ThemeColor status_bar_bg;            /* Status bar background */
    ThemeColor status_bar_fg;            /* Status bar foreground */

    struct Theme *next;                  /* Linked list of themes */
} Theme;

/* Theme registry */
extern Theme *theme_list;
extern Theme *current_theme;

/* Initialize theme system */
void theme_init(void);

/* Create a new theme */
Theme *theme_create(const char *name);

/* Load theme from JSON file */
Theme *theme_load_from_file(const char *filepath);

/* Save theme to JSON file */
int theme_save_to_file(Theme *theme, const char *filepath);

/* Register a theme in the global registry */
void theme_register(Theme *theme);

/* Find theme by name */
Theme *theme_find(const char *name);

/* Set the active theme */
void theme_set_active(Theme *theme);

/* Get current active theme */
Theme *theme_get_active(void);

/* Set color for a specific highlight type in a theme */
void theme_set_color(Theme *theme, HighlightType type, ThemeColor fg, ThemeColor bg, uint8_t attrs);

/* Set UI color */
void theme_set_ui_color(Theme *theme, const char *element, ThemeColor color);

/* Helper: Create RGB color */
ThemeColor theme_rgb(uint8_t r, uint8_t g, uint8_t b);

/* Helper: Create ANSI color */
ThemeColor theme_ansi(Color ansi_color);

/* Helper: Create hex color from string like "#FF0000" */
ThemeColor theme_hex(const char *hex_string);

/* Convert ThemeColor to ANSI escape sequence */
void theme_color_to_ansi(ThemeColor tc, char *buf, size_t bufsize, bool foreground);

/* Convert ThemeColorPair to ANSI escape sequence */
void theme_colorpair_to_ansi(ThemeColorPair tcp, char *buf, size_t bufsize);

/* Free theme resources */
void theme_free(Theme *theme);

/* Built-in themes */
Theme *theme_create_default(void);
Theme *theme_create_monokai(void);
Theme *theme_create_solarized_dark(void);
Theme *theme_create_solarized_light(void);
Theme *theme_create_nord(void);
Theme *theme_create_dracula(void);
Theme *theme_create_gruvbox_dark(void);
Theme *theme_create_gruvbox_light(void);
Theme *theme_create_one_dark(void);
Theme *theme_create_github_light(void);

#endif /* THEME_H */
