#define _POSIX_C_SOURCE 200809L
#include "theme.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Global theme registry */
Theme *theme_list = NULL;
Theme *current_theme = NULL;

void theme_init(void) {
    /* Register built-in themes */
    theme_register(theme_create_default());
    theme_register(theme_create_monokai());
    theme_register(theme_create_solarized_dark());
    theme_register(theme_create_solarized_light());
    theme_register(theme_create_nord());
    theme_register(theme_create_dracula());
    theme_register(theme_create_gruvbox_dark());
    theme_register(theme_create_gruvbox_light());
    theme_register(theme_create_one_dark());
    theme_register(theme_create_github_light());

    /* Set default theme */
    current_theme = theme_find("default");
}

Theme *theme_create(const char *name) {
    Theme *theme = calloc(1, sizeof(Theme));
    if (!theme) return NULL;

    theme->name = strdup(name);
    theme->author = NULL;
    theme->description = NULL;
    theme->dark_mode = true;
    theme->next = NULL;

    /* Set sensible defaults */
    theme->background = theme_ansi(COLOR_DEFAULT);
    theme->foreground = theme_ansi(COLOR_WHITE);
    theme->selection_bg = theme_ansi(COLOR_BLUE);
    theme->selection_fg = theme_ansi(COLOR_WHITE);
    theme->line_number = theme_ansi(COLOR_BRIGHT_BLACK);
    theme->current_line_bg = theme_ansi(COLOR_DEFAULT);
    theme->cursor = theme_ansi(COLOR_WHITE);
    theme->status_bar_bg = theme_ansi(COLOR_BLUE);
    theme->status_bar_fg = theme_ansi(COLOR_WHITE);

    return theme;
}

void theme_register(Theme *theme) {
    if (!theme) return;
    theme->next = theme_list;
    theme_list = theme;
}

Theme *theme_find(const char *name) {
    for (Theme *t = theme_list; t; t = t->next) {
        if (strcmp(t->name, name) == 0) {
            return t;
        }
    }
    return NULL;
}

void theme_set_active(Theme *theme) {
    current_theme = theme;
}

Theme *theme_get_active(void) {
    return current_theme;
}

void theme_set_color(Theme *theme, HighlightType type, ThemeColor fg, ThemeColor bg, uint8_t attrs) {
    if (!theme || type < 0 || type >= HL_MAX) return;

    theme->colors[type].fg = fg;
    theme->colors[type].bg = bg;
    theme->colors[type].attrs = attrs;
}

ThemeColor theme_rgb(uint8_t r, uint8_t g, uint8_t b) {
    ThemeColor tc;
    tc.is_rgb = true;
    tc.rgb.r = r;
    tc.rgb.g = g;
    tc.rgb.b = b;
    return tc;
}

ThemeColor theme_ansi(Color ansi_color) {
    ThemeColor tc;
    tc.is_rgb = false;
    tc.ansi = ansi_color;
    return tc;
}

ThemeColor theme_hex(const char *hex_string) {
    ThemeColor tc;
    tc.is_rgb = true;

    if (!hex_string || hex_string[0] != '#') {
        /* Invalid format, return white */
        tc.rgb.r = tc.rgb.g = tc.rgb.b = 255;
        return tc;
    }

    unsigned int rgb;
    if (sscanf(hex_string + 1, "%x", &rgb) == 1) {
        tc.rgb.r = (rgb >> 16) & 0xFF;
        tc.rgb.g = (rgb >> 8) & 0xFF;
        tc.rgb.b = rgb & 0xFF;
    } else {
        /* Parse error, return white */
        tc.rgb.r = tc.rgb.g = tc.rgb.b = 255;
    }

    return tc;
}

void theme_color_to_ansi(ThemeColor tc, char *buf, size_t bufsize, bool foreground) {
    int pos = 0;

    if (tc.is_rgb) {
        /* TrueColor (24-bit RGB): ESC[38;2;R;G;Bm or ESC[48;2;R;G;Bm */
        if (foreground) {
            pos = snprintf(buf, bufsize, "\x1b[38;2;%d;%d;%dm", tc.rgb.r, tc.rgb.g, tc.rgb.b);
        } else {
            pos = snprintf(buf, bufsize, "\x1b[48;2;%d;%d;%dm", tc.rgb.r, tc.rgb.g, tc.rgb.b);
        }
    } else {
        /* ANSI color */
        if (tc.ansi == COLOR_DEFAULT) {
            pos = snprintf(buf, bufsize, "\x1b[%dm", foreground ? 39 : 49);
        } else if (tc.ansi >= COLOR_BRIGHT_BLACK) {
            /* Bright colors (90-97 for fg, 100-107 for bg) */
            int code = foreground ? (90 + (tc.ansi - COLOR_BRIGHT_BLACK)) : (100 + (tc.ansi - COLOR_BRIGHT_BLACK));
            pos = snprintf(buf, bufsize, "\x1b[%dm", code);
        } else {
            /* Normal colors (30-37 for fg, 40-47 for bg) */
            int code = foreground ? (30 + tc.ansi) : (40 + tc.ansi);
            pos = snprintf(buf, bufsize, "\x1b[%dm", code);
        }
    }
}

void theme_colorpair_to_ansi(ThemeColorPair tcp, char *buf, size_t bufsize) {
    int pos = 0;

    /* Start escape sequence */
    pos += snprintf(buf + pos, bufsize - pos, "\x1b[");

    /* Add text attributes */
    if (tcp.attrs & (1 << ATTR_BOLD)) {
        pos += snprintf(buf + pos, bufsize - pos, "1;");
    }
    if (tcp.attrs & (1 << ATTR_DIM)) {
        pos += snprintf(buf + pos, bufsize - pos, "2;");
    }
    if (tcp.attrs & (1 << ATTR_ITALIC)) {
        pos += snprintf(buf + pos, bufsize - pos, "3;");
    }
    if (tcp.attrs & (1 << ATTR_UNDERLINE)) {
        pos += snprintf(buf + pos, bufsize - pos, "4;");
    }
    if (tcp.attrs & (1 << ATTR_REVERSE)) {
        pos += snprintf(buf + pos, bufsize - pos, "7;");
    }

    /* Foreground color */
    if (tcp.fg.is_rgb) {
        pos += snprintf(buf + pos, bufsize - pos, "38;2;%d;%d;%d;",
                       tcp.fg.rgb.r, tcp.fg.rgb.g, tcp.fg.rgb.b);
    } else if (tcp.fg.ansi != COLOR_DEFAULT) {
        if (tcp.fg.ansi >= COLOR_BRIGHT_BLACK) {
            pos += snprintf(buf + pos, bufsize - pos, "%d;", 90 + (tcp.fg.ansi - COLOR_BRIGHT_BLACK));
        } else {
            pos += snprintf(buf + pos, bufsize - pos, "%d;", 30 + tcp.fg.ansi);
        }
    }

    /* Background color */
    if (tcp.bg.is_rgb) {
        pos += snprintf(buf + pos, bufsize - pos, "48;2;%d;%d;%d;",
                       tcp.bg.rgb.r, tcp.bg.rgb.g, tcp.bg.rgb.b);
    } else if (tcp.bg.ansi != COLOR_DEFAULT) {
        if (tcp.bg.ansi >= COLOR_BRIGHT_BLACK) {
            pos += snprintf(buf + pos, bufsize - pos, "%d;", 100 + (tcp.bg.ansi - COLOR_BRIGHT_BLACK));
        } else {
            pos += snprintf(buf + pos, bufsize - pos, "%d;", 40 + tcp.bg.ansi);
        }
    }

    /* Remove trailing semicolon if present */
    if (pos > 2 && buf[pos - 1] == ';') {
        pos--;
    }

    /* Close escape sequence */
    snprintf(buf + pos, bufsize - pos, "m");
}

void theme_free(Theme *theme) {
    if (!theme) return;
    if (theme->name) free(theme->name);
    if (theme->author) free(theme->author);
    if (theme->description) free(theme->description);
    free(theme);
}

/* ========== Built-in Themes ========== */

Theme *theme_create_default(void) {
    Theme *theme = theme_create("default");
    if (!theme) return NULL;

    theme->description = strdup("Default OCCE theme with ANSI colors");
    theme->dark_mode = true;

    /* Syntax highlighting colors (from original colors.c) */
    theme_set_color(theme, HL_NORMAL, theme_ansi(COLOR_DEFAULT), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_KEYWORD, theme_ansi(COLOR_MAGENTA), theme_ansi(COLOR_DEFAULT), 1 << ATTR_BOLD);
    theme_set_color(theme, HL_TYPE, theme_ansi(COLOR_CYAN), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_STRING, theme_ansi(COLOR_YELLOW), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_NUMBER, theme_ansi(COLOR_BRIGHT_CYAN), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_COMMENT, theme_ansi(COLOR_BRIGHT_BLACK), theme_ansi(COLOR_DEFAULT), 1 << ATTR_ITALIC);
    theme_set_color(theme, HL_OPERATOR, theme_ansi(COLOR_WHITE), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_FUNCTION, theme_ansi(COLOR_BLUE), theme_ansi(COLOR_DEFAULT), 1 << ATTR_BOLD);
    theme_set_color(theme, HL_VARIABLE, theme_ansi(COLOR_WHITE), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_CONSTANT, theme_ansi(COLOR_BRIGHT_MAGENTA), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_PREPROCESSOR, theme_ansi(COLOR_BRIGHT_RED), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_ERROR, theme_ansi(COLOR_RED), theme_ansi(COLOR_DEFAULT), 1 << ATTR_REVERSE);

    return theme;
}

Theme *theme_create_monokai(void) {
    Theme *theme = theme_create("monokai");
    if (!theme) return NULL;

    theme->description = strdup("Monokai - Popular dark theme");
    theme->author = strdup("Wimer Hazenberg");
    theme->dark_mode = true;

    /* Monokai TrueColor palette */
    theme->background = theme_hex("#272822");
    theme->foreground = theme_hex("#F8F8F2");
    theme->selection_bg = theme_hex("#49483E");
    theme->line_number = theme_hex("#90908A");
    theme->current_line_bg = theme_hex("#3E3D32");
    theme->cursor = theme_hex("#F8F8F0");

    theme_set_color(theme, HL_NORMAL, theme_hex("#F8F8F2"), theme_hex("#272822"), 0);
    theme_set_color(theme, HL_KEYWORD, theme_hex("#F92672"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_BOLD);
    theme_set_color(theme, HL_TYPE, theme_hex("#66D9EF"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_ITALIC);
    theme_set_color(theme, HL_STRING, theme_hex("#E6DB74"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_NUMBER, theme_hex("#AE81FF"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_COMMENT, theme_hex("#75715E"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_ITALIC);
    theme_set_color(theme, HL_OPERATOR, theme_hex("#F92672"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_FUNCTION, theme_hex("#A6E22E"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_VARIABLE, theme_hex("#F8F8F2"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_CONSTANT, theme_hex("#AE81FF"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_PREPROCESSOR, theme_hex("#F92672"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_ERROR, theme_hex("#F92672"), theme_hex("#1E0010"), 0);

    return theme;
}

Theme *theme_create_solarized_dark(void) {
    Theme *theme = theme_create("solarized-dark");
    if (!theme) return NULL;

    theme->description = strdup("Solarized Dark - Precision colors for machines and people");
    theme->author = strdup("Ethan Schoonover");
    theme->dark_mode = true;

    theme->background = theme_hex("#002B36");
    theme->foreground = theme_hex("#839496");
    theme->selection_bg = theme_hex("#073642");
    theme->line_number = theme_hex("#586E75");
    theme->cursor = theme_hex("#839496");

    theme_set_color(theme, HL_NORMAL, theme_hex("#839496"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_KEYWORD, theme_hex("#859900"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_BOLD);
    theme_set_color(theme, HL_TYPE, theme_hex("#B58900"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_STRING, theme_hex("#2AA198"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_NUMBER, theme_hex("#D33682"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_COMMENT, theme_hex("#586E75"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_ITALIC);
    theme_set_color(theme, HL_OPERATOR, theme_hex("#859900"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_FUNCTION, theme_hex("#268BD2"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_VARIABLE, theme_hex("#839496"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_CONSTANT, theme_hex("#CB4B16"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_PREPROCESSOR, theme_hex("#CB4B16"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_ERROR, theme_hex("#DC322F"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_REVERSE);

    return theme;
}

Theme *theme_create_solarized_light(void) {
    Theme *theme = theme_create("solarized-light");
    if (!theme) return NULL;

    theme->description = strdup("Solarized Light - Precision colors for machines and people");
    theme->author = strdup("Ethan Schoonover");
    theme->dark_mode = false;

    theme->background = theme_hex("#FDF6E3");
    theme->foreground = theme_hex("#657B83");
    theme->selection_bg = theme_hex("#EEE8D5");
    theme->line_number = theme_hex("#93A1A1");
    theme->cursor = theme_hex("#657B83");

    theme_set_color(theme, HL_NORMAL, theme_hex("#657B83"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_KEYWORD, theme_hex("#859900"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_BOLD);
    theme_set_color(theme, HL_TYPE, theme_hex("#B58900"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_STRING, theme_hex("#2AA198"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_NUMBER, theme_hex("#D33682"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_COMMENT, theme_hex("#93A1A1"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_ITALIC);
    theme_set_color(theme, HL_OPERATOR, theme_hex("#859900"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_FUNCTION, theme_hex("#268BD2"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_VARIABLE, theme_hex("#657B83"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_CONSTANT, theme_hex("#CB4B16"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_PREPROCESSOR, theme_hex("#CB4B16"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_ERROR, theme_hex("#DC322F"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_REVERSE);

    return theme;
}

Theme *theme_create_nord(void) {
    Theme *theme = theme_create("nord");
    if (!theme) return NULL;

    theme->description = strdup("Nord - Arctic, north-bluish color palette");
    theme->author = strdup("Arctic Ice Studio");
    theme->dark_mode = true;

    theme->background = theme_hex("#2E3440");
    theme->foreground = theme_hex("#D8DEE9");
    theme->selection_bg = theme_hex("#434C5E");
    theme->line_number = theme_hex("#4C566A");
    theme->cursor = theme_hex("#D8DEE9");

    theme_set_color(theme, HL_NORMAL, theme_hex("#D8DEE9"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_KEYWORD, theme_hex("#81A1C1"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_BOLD);
    theme_set_color(theme, HL_TYPE, theme_hex("#8FBCBB"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_STRING, theme_hex("#A3BE8C"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_NUMBER, theme_hex("#B48EAD"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_COMMENT, theme_hex("#616E88"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_ITALIC);
    theme_set_color(theme, HL_OPERATOR, theme_hex("#81A1C1"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_FUNCTION, theme_hex("#88C0D0"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_VARIABLE, theme_hex("#D8DEE9"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_CONSTANT, theme_hex("#D08770"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_PREPROCESSOR, theme_hex("#5E81AC"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_ERROR, theme_hex("#BF616A"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_REVERSE);

    return theme;
}

Theme *theme_create_dracula(void) {
    Theme *theme = theme_create("dracula");
    if (!theme) return NULL;

    theme->description = strdup("Dracula - Dark theme with vibrant colors");
    theme->author = strdup("Zeno Rocha");
    theme->dark_mode = true;

    theme->background = theme_hex("#282A36");
    theme->foreground = theme_hex("#F8F8F2");
    theme->selection_bg = theme_hex("#44475A");
    theme->line_number = theme_hex("#6272A4");
    theme->cursor = theme_hex("#F8F8F2");

    theme_set_color(theme, HL_NORMAL, theme_hex("#F8F8F2"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_KEYWORD, theme_hex("#FF79C6"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_BOLD);
    theme_set_color(theme, HL_TYPE, theme_hex("#8BE9FD"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_ITALIC);
    theme_set_color(theme, HL_STRING, theme_hex("#F1FA8C"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_NUMBER, theme_hex("#BD93F9"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_COMMENT, theme_hex("#6272A4"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_ITALIC);
    theme_set_color(theme, HL_OPERATOR, theme_hex("#FF79C6"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_FUNCTION, theme_hex("#50FA7B"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_VARIABLE, theme_hex("#F8F8F2"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_CONSTANT, theme_hex("#BD93F9"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_PREPROCESSOR, theme_hex("#FF79C6"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_ERROR, theme_hex("#FF5555"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_REVERSE);

    return theme;
}

Theme *theme_create_gruvbox_dark(void) {
    Theme *theme = theme_create("gruvbox-dark");
    if (!theme) return NULL;

    theme->description = strdup("Gruvbox Dark - Retro groove color scheme");
    theme->author = strdup("Pavel Pertsev");
    theme->dark_mode = true;

    theme->background = theme_hex("#282828");
    theme->foreground = theme_hex("#EBDBB2");
    theme->selection_bg = theme_hex("#504945");
    theme->line_number = theme_hex("#7C6F64");
    theme->cursor = theme_hex("#EBDBB2");

    theme_set_color(theme, HL_NORMAL, theme_hex("#EBDBB2"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_KEYWORD, theme_hex("#FB4934"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_BOLD);
    theme_set_color(theme, HL_TYPE, theme_hex("#FABD2F"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_STRING, theme_hex("#B8BB26"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_NUMBER, theme_hex("#D3869B"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_COMMENT, theme_hex("#928374"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_ITALIC);
    theme_set_color(theme, HL_OPERATOR, theme_hex("#FE8019"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_FUNCTION, theme_hex("#8EC07C"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_VARIABLE, theme_hex("#EBDBB2"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_CONSTANT, theme_hex("#D3869B"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_PREPROCESSOR, theme_hex("#FE8019"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_ERROR, theme_hex("#FB4934"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_REVERSE);

    return theme;
}

Theme *theme_create_gruvbox_light(void) {
    Theme *theme = theme_create("gruvbox-light");
    if (!theme) return NULL;

    theme->description = strdup("Gruvbox Light - Retro groove color scheme");
    theme->author = strdup("Pavel Pertsev");
    theme->dark_mode = false;

    theme->background = theme_hex("#FBF1C7");
    theme->foreground = theme_hex("#3C3836");
    theme->selection_bg = theme_hex("#EBDBB2");
    theme->line_number = theme_hex("#928374");
    theme->cursor = theme_hex("#3C3836");

    theme_set_color(theme, HL_NORMAL, theme_hex("#3C3836"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_KEYWORD, theme_hex("#9D0006"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_BOLD);
    theme_set_color(theme, HL_TYPE, theme_hex("#B57614"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_STRING, theme_hex("#79740E"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_NUMBER, theme_hex("#8F3F71"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_COMMENT, theme_hex("#928374"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_ITALIC);
    theme_set_color(theme, HL_OPERATOR, theme_hex("#AF3A03"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_FUNCTION, theme_hex("#427B58"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_VARIABLE, theme_hex("#3C3836"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_CONSTANT, theme_hex("#8F3F71"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_PREPROCESSOR, theme_hex("#AF3A03"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_ERROR, theme_hex("#9D0006"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_REVERSE);

    return theme;
}

Theme *theme_create_one_dark(void) {
    Theme *theme = theme_create("one-dark");
    if (!theme) return NULL;

    theme->description = strdup("One Dark - Atom's iconic dark theme");
    theme->author = strdup("Atom");
    theme->dark_mode = true;

    theme->background = theme_hex("#282C34");
    theme->foreground = theme_hex("#ABB2BF");
    theme->selection_bg = theme_hex("#3E4451");
    theme->line_number = theme_hex("#5C6370");
    theme->cursor = theme_hex("#528BFF");

    theme_set_color(theme, HL_NORMAL, theme_hex("#ABB2BF"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_KEYWORD, theme_hex("#C678DD"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_BOLD);
    theme_set_color(theme, HL_TYPE, theme_hex("#E5C07B"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_STRING, theme_hex("#98C379"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_NUMBER, theme_hex("#D19A66"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_COMMENT, theme_hex("#5C6370"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_ITALIC);
    theme_set_color(theme, HL_OPERATOR, theme_hex("#56B6C2"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_FUNCTION, theme_hex("#61AFEF"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_VARIABLE, theme_hex("#E06C75"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_CONSTANT, theme_hex("#D19A66"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_PREPROCESSOR, theme_hex("#C678DD"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_ERROR, theme_hex("#E06C75"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_REVERSE);

    return theme;
}

Theme *theme_create_github_light(void) {
    Theme *theme = theme_create("github-light");
    if (!theme) return NULL;

    theme->description = strdup("GitHub Light - GitHub's light theme");
    theme->author = strdup("GitHub");
    theme->dark_mode = false;

    theme->background = theme_hex("#FFFFFF");
    theme->foreground = theme_hex("#24292E");
    theme->selection_bg = theme_hex("#C8E1FF");
    theme->line_number = theme_hex("#6A737D");
    theme->cursor = theme_hex("#24292E");

    theme_set_color(theme, HL_NORMAL, theme_hex("#24292E"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_KEYWORD, theme_hex("#D73A49"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_BOLD);
    theme_set_color(theme, HL_TYPE, theme_hex("#6F42C1"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_STRING, theme_hex("#032F62"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_NUMBER, theme_hex("#005CC5"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_COMMENT, theme_hex("#6A737D"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_ITALIC);
    theme_set_color(theme, HL_OPERATOR, theme_hex("#D73A49"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_FUNCTION, theme_hex("#6F42C1"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_VARIABLE, theme_hex("#24292E"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_CONSTANT, theme_hex("#005CC5"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_PREPROCESSOR, theme_hex("#D73A49"), theme_ansi(COLOR_DEFAULT), 0);
    theme_set_color(theme, HL_ERROR, theme_hex("#B31D28"), theme_ansi(COLOR_DEFAULT), 1 << ATTR_REVERSE);

    return theme;
}
