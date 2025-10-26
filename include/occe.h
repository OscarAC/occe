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
typedef struct Terminal Terminal;
typedef struct KeyMap KeyMap;
typedef struct GitRepo GitRepo;

/* Editor modes */
typedef enum {
    MODE_NORMAL,
    MODE_INSERT,
    MODE_COMMAND,
    MODE_VISUAL
} EditorMode;

/* Main editor state */
struct Editor {
    Terminal *term;
    Buffer **buffers;
    size_t buffer_count;
    Window *root_window;
    Window *active_window;
    bool running;
    void *lua_state;

    /* Mode and command state */
    EditorMode mode;
    char command_buf[256];
    size_t command_len;
    bool window_command_mode;  /* True after Ctrl+W pressed */

    /* Status message */
    char status_msg[256];
    size_t status_len;

    /* Keybinding map */
    KeyMap *keymap;

    /* Display options */
    bool show_line_numbers;

    /* Clipboard */
    char *clipboard;
    size_t clipboard_len;

    /* Git integration */
    GitRepo *git_repo;

    /* Config directory path */
    char *config_dir;
};

/* Initialize and run editor */
Editor *editor_create(void);
void editor_destroy(Editor *ed);
int editor_run(Editor *ed);
void editor_quit(Editor *ed);

/* Mode and command functions */
void editor_set_mode(Editor *ed, EditorMode mode);
void editor_execute_command(Editor *ed, const char *cmd);
void editor_set_status(Editor *ed, const char *msg);

#endif /* OCCE_H */
