#ifndef OCCE_H
#define OCCE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Version */
#define OCCE_VERSION "0.1.0"

/* Forward declarations */
typedef struct Editor Editor;
typedef struct Buffer Buffer;
typedef struct Window Window;
typedef struct TabGroup TabGroup;
typedef struct Terminal Terminal;
typedef struct KeyMap KeyMap;

/* Main editor state */
struct Editor {
    Terminal *term;
    Buffer **buffers;
    size_t buffer_count;

    /* Window/Tab management */
    TabGroup *tab_groups;      /* Linked list of tabs */
    TabGroup *active_tab;      /* Currently visible tab */
    Window *root_window;       /* Active tab's root (convenience) */
    Window *active_window;     /* Currently focused window (convenience) */
    int next_window_id;        /* For unique window IDs */
    int next_tab_id;           /* For unique tab IDs */

    bool running;
    void *lua_state;

    /* Status message */
    char status_msg[256];
    size_t status_len;

    /* Keybinding map */
    KeyMap *keymap;

    /* Display options */
    bool show_line_numbers;

    /* Tab settings */
    int tab_width;       /* Number of spaces per tab */
    bool use_spaces;     /* Use spaces instead of tabs */

    /* Clipboard */
    char *clipboard;
    size_t clipboard_len;

    /* Config directory path */
    char *config_dir;
};

/* Initialize and run editor */
Editor *editor_create(void);
void editor_destroy(Editor *ed);
int editor_run(Editor *ed);
void editor_quit(Editor *ed);
void editor_set_status(Editor *ed, const char *msg);

#endif /* OCCE_H */
