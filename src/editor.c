#define _POSIX_C_SOURCE 200809L
#include "occe.h"
#include "terminal.h"
#include "buffer.h"
#include "window.h"
#include "lua_bridge.h"
#include "keybind.h"
#include "syntax.h"
#include "colors.h"
#include "undo.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

/* Get the config directory path (~/.config/occe) */
static char *get_config_dir(void) {
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }

    if (!home) return NULL;

    size_t len = strlen(home) + strlen("/.config/occe") + 1;
    char *config_dir = malloc(len);
    if (!config_dir) return NULL;

    snprintf(config_dir, len, "%s/.config/occe", home);
    return config_dir;
}

Editor *editor_create(void) {
    Editor *ed = malloc(sizeof(Editor));
    if (!ed) return NULL;

    ed->term = terminal_create();
    if (!ed->term) {
        free(ed);
        return NULL;
    }

    ed->buffers = NULL;
    ed->buffer_count = 0;

    /* Initialize window/tab management */
    ed->tab_groups = NULL;
    ed->active_tab = NULL;
    ed->root_window = NULL;
    ed->active_window = NULL;
    ed->next_window_id = 1;
    ed->next_tab_id = 1;

    ed->running = true;
    ed->lua_state = NULL;

    /* Initialize window command state */
    ed->window_command_mode = false;
    ed->status_len = 0;
    ed->status_msg[0] = '\0';

    /* Initialize keybinding map */
    ed->keymap = keymap_create();
    if (!ed->keymap) {
        terminal_destroy(ed->term);
        free(ed);
        return NULL;
    }

    /* Initialize display options */
    ed->show_line_numbers = true;  /* Line numbers on by default */

    /* Initialize clipboard */
    ed->clipboard = NULL;
    ed->clipboard_len = 0;

    /* Get config directory */
    ed->config_dir = get_config_dir();

    /* Initialize colors and syntax */
    colors_init();
    syntax_init();

    /* Initialize Lua */
    if (lua_bridge_init(ed) != 0) {
        keymap_destroy(ed->keymap);
        terminal_destroy(ed->term);
        if (ed->config_dir) free(ed->config_dir);
        free(ed);
        return NULL;
    }

    /* Load user configuration - try local init.lua first (dev mode), then config dir */
    if (lua_bridge_load_plugin(ed, "./init.lua") != 0) {
        /* Local init.lua not found, try config directory */
        if (ed->config_dir) {
            char init_path[512];
            snprintf(init_path, sizeof(init_path), "%s/init.lua", ed->config_dir);
            lua_bridge_load_plugin(ed, init_path);
        }
    }

    return ed;
}

void editor_destroy(Editor *ed) {
    if (!ed) return;

    /* Clean up Lua */
    lua_bridge_cleanup(ed);

    /* Clean up keybindings */
    keymap_destroy(ed->keymap);

    /* Clean up tab groups */
    TabGroup *current_tab = ed->tab_groups;
    while (current_tab) {
        TabGroup *next = current_tab->next;
        tabgroup_destroy(current_tab);
        current_tab = next;
    }

    /* Clean up windows (if not using tabs) */
    if (ed->root_window && !ed->tab_groups) {
        window_destroy(ed->root_window);
    }

    /* Clean up buffers */
    for (size_t i = 0; i < ed->buffer_count; i++) {
        buffer_destroy(ed->buffers[i]);
    }
    if (ed->buffers) free(ed->buffers);

    /* Clean up clipboard */
    if (ed->clipboard) free(ed->clipboard);

    /* Clean up config directory path */
    if (ed->config_dir) free(ed->config_dir);

    /* Clean up terminal */
    terminal_destroy(ed->term);

    free(ed);
}

void editor_quit(Editor *ed) {
    ed->running = false;
}

void editor_set_status(Editor *ed, const char *msg) {
    size_t len = strlen(msg);
    if (len >= sizeof(ed->status_msg)) {
        len = sizeof(ed->status_msg) - 1;
    }
    memcpy(ed->status_msg, msg, len);
    ed->status_msg[len] = '\0';
    ed->status_len = len;
}

/* Helper function to add a buffer to the editor */
static void editor_add_buffer(Editor *ed, Buffer *buf) {
    if (!buf) return;

    Buffer **new_buffers = realloc(ed->buffers, sizeof(Buffer *) * (ed->buffer_count + 1));
    if (!new_buffers) return;

    ed->buffers = new_buffers;
    ed->buffers[ed->buffer_count++] = buf;
}

/* Helper function to split a window */
static void editor_split_window(Editor *ed, WindowType split_type, Buffer *new_buffer) {
    if (!ed->active_window || !new_buffer) return;

    Window *current = ed->active_window;
    if (current->type != WINDOW_LEAF) return;  /* Can only split leaf windows */

    /* Save current window properties */
    int x = current->x;
    int y = current->y;
    int w = current->width;
    int h = current->height;
    Buffer *old_buffer = current->content.buffer;

    /* Create two new leaf windows */
    Window *win1 = window_create_leaf(old_buffer, 0, 0, 0, 0);
    Window *win2 = window_create_leaf(new_buffer, 0, 0, 0, 0);

    if (!win1 || !win2) {
        if (win1) free(win1);
        if (win2) free(win2);
        return;
    }

    /* Convert current window to a split */
    current->type = split_type;
    current->content.buffer = NULL;
    current->left = win1;
    current->right = win2;
    current->split_ratio = 0.5f;

    /* Resize to distribute space */
    window_resize(current, x, y, w, h);

    /* Set active window to the new window */
    ed->active_window = win2;
}

void editor_execute_command(Editor *ed, const char *cmd) {
    if (!cmd || !cmd[0]) return;

    /* Handle built-in commands */
    if (strcmp(cmd, "q") == 0 || strcmp(cmd, "quit") == 0) {
        editor_quit(ed);
        return;
    }

    if (strcmp(cmd, "w") == 0 || strcmp(cmd, "write") == 0) {
        if (ed->active_window && ed->active_window->content.buffer) {
            if (buffer_save(ed->active_window->content.buffer) == 0) {
                editor_set_status(ed, "File saved");
            } else {
                editor_set_status(ed, "Error saving file");
            }
        }
        return;
    }

    if (strcmp(cmd, "wq") == 0) {
        if (ed->active_window && ed->active_window->content.buffer) {
            buffer_save(ed->active_window->content.buffer);
        }
        editor_quit(ed);
        return;
    }

    /* Handle set commands */
    if (strcmp(cmd, "set number") == 0 || strcmp(cmd, "set nu") == 0) {
        ed->show_line_numbers = true;
        editor_set_status(ed, "Line numbers enabled");
        return;
    }

    if (strcmp(cmd, "set nonumber") == 0 || strcmp(cmd, "set nonu") == 0) {
        ed->show_line_numbers = false;
        editor_set_status(ed, "Line numbers disabled");
        return;
    }

    /* Handle buffer list */
    if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "buffers") == 0) {
        char msg[512];
        int offset = 0;
        offset += snprintf(msg + offset, sizeof(msg) - offset, "Buffers: ");
        for (size_t i = 0; i < ed->buffer_count && offset < (int)sizeof(msg) - 50; i++) {
            const char *name = ed->buffers[i]->filename ? ed->buffers[i]->filename : "[No Name]";
            const char *modified = ed->buffers[i]->modified ? "[+]" : "";
            const char *active = (ed->active_window && ed->active_window->content.buffer == ed->buffers[i]) ? "*" : " ";
            offset += snprintf(msg + offset, sizeof(msg) - offset, "%s%zu:%s%s ",
                             active, i + 1, name, modified);
        }
        editor_set_status(ed, msg);
        return;
    }

    /* Handle window management */
    if (strcmp(cmd, "only") == 0) {
        if (ed->active_window && ed->root_window) {
            Window *new_root = window_only(ed->root_window, ed->active_window);
            if (new_root) {
                ed->root_window = new_root;
                ed->active_window = new_root;
                /* Resize to fill screen */
                window_resize(ed->root_window, 0, 0, ed->term->cols, ed->term->rows - 1);
                editor_set_status(ed, "Closed all other windows");
            }
        }
        return;
    }

    /* Handle split commands */
    if (strncmp(cmd, "split", 5) == 0 || strncmp(cmd, "sp", 2) == 0) {
        const char *filename = NULL;

        /* Check for filename argument */
        if (strncmp(cmd, "split ", 6) == 0) {
            filename = cmd + 6;
            while (*filename == ' ') filename++;  /* Skip spaces */
            if (*filename == '\0') filename = NULL;
        } else if (strncmp(cmd, "sp ", 3) == 0) {
            filename = cmd + 3;
            while (*filename == ' ') filename++;
            if (*filename == '\0') filename = NULL;
        }

        /* Create new buffer */
        Buffer *new_buf = buffer_create();
        if (new_buf) {
            if (filename) {
                /* Try to open file */
                if (buffer_open(new_buf, filename) == -1) {
                    /* File doesn't exist, create new with filename */
                    new_buf->filename = strdup(filename);
                    buffer_append_row(new_buf, "", 0);
                }
            } else {
                /* No filename - create empty buffer WITHOUT filename to prevent data loss */
                /* Copy content from current buffer but don't share the filename */
                if (ed->active_window && ed->active_window->content.buffer) {
                    Buffer *src = ed->active_window->content.buffer;
                    for (size_t i = 0; i < src->num_rows; i++) {
                        buffer_append_row(new_buf, src->rows[i].data, src->rows[i].size);
                    }
                } else {
                    buffer_append_row(new_buf, "", 0);
                }
                /* Leave new_buf->filename as NULL - user must save with new name */
            }

            editor_add_buffer(ed, new_buf);
            editor_split_window(ed, WINDOW_SPLIT_H, new_buf);
            editor_set_status(ed, "Window split horizontally");
        }
        return;
    }

    if (strncmp(cmd, "vsplit", 6) == 0 || strncmp(cmd, "vsp", 3) == 0) {
        const char *filename = NULL;

        /* Check for filename argument */
        if (strncmp(cmd, "vsplit ", 7) == 0) {
            filename = cmd + 7;
            while (*filename == ' ') filename++;
            if (*filename == '\0') filename = NULL;
        } else if (strncmp(cmd, "vsp ", 4) == 0) {
            filename = cmd + 4;
            while (*filename == ' ') filename++;
            if (*filename == '\0') filename = NULL;
        }

        /* Create new buffer */
        Buffer *new_buf = buffer_create();
        if (new_buf) {
            if (filename) {
                /* Try to open file */
                if (buffer_open(new_buf, filename) == -1) {
                    /* File doesn't exist, create new with filename */
                    new_buf->filename = strdup(filename);
                    buffer_append_row(new_buf, "", 0);
                }
            } else {
                /* No filename - create empty buffer WITHOUT filename to prevent data loss */
                /* Copy content from current buffer but don't share the filename */
                if (ed->active_window && ed->active_window->content.buffer) {
                    Buffer *src = ed->active_window->content.buffer;
                    for (size_t i = 0; i < src->num_rows; i++) {
                        buffer_append_row(new_buf, src->rows[i].data, src->rows[i].size);
                    }
                } else {
                    buffer_append_row(new_buf, "", 0);
                }
                /* Leave new_buf->filename as NULL - user must save with new name */
            }

            editor_add_buffer(ed, new_buf);
            editor_split_window(ed, WINDOW_SPLIT_V, new_buf);
            editor_set_status(ed, "Window split vertically");
        }
        return;
    }

    /* Try to execute as Lua code */
    if (lua_bridge_exec(ed, cmd) == 0) {
        editor_set_status(ed, "Command executed");
    } else {
        char msg[256];
        snprintf(msg, sizeof(msg), "Unknown command: %s", cmd);
        editor_set_status(ed, msg);
    }
}

static void editor_process_keypress(Editor *ed, int key) {

    /* Handle window command mode */
    if (ed->window_command_mode) {
        ed->window_command_mode = false;  /* Reset after handling */

        /* Window navigation */
        switch (key) {
            case 'h':
            case KEY_ARROW_LEFT:
            case 'j':
            case KEY_ARROW_DOWN:
            case 'k':
            case KEY_ARROW_UP:
            case 'l':
            case KEY_ARROW_RIGHT: {
                /* Navigate to next window (simplified - just cycle through) */
                Window *next = window_get_next_leaf(ed->root_window, ed->active_window);
                if (next && next != ed->active_window) {
                    ed->active_window = next;
                    editor_set_status(ed, "Switched window");
                }
                break;
            }

            case 'w':
                /* Ctrl+W w - cycle to next window */
                {
                    Window *next = window_get_next_leaf(ed->root_window, ed->active_window);
                    if (next && next != ed->active_window) {
                        ed->active_window = next;
                        editor_set_status(ed, "Switched to next window");
                    }
                }
                break;

            case 'W':
                /* Ctrl+W W - cycle to previous window */
                {
                    Window *prev = window_get_prev_leaf(ed->root_window, ed->active_window);
                    if (prev && prev != ed->active_window) {
                        ed->active_window = prev;
                        editor_set_status(ed, "Switched to previous window");
                    }
                }
                break;

            default:
                /* Unknown window command */
                break;
        }
        return;
    }

    /* Check for Ctrl+W to enter window command mode */
    if (key == KEY_CTRL_W) {
        ed->window_command_mode = true;
        editor_set_status(ed, "-- WINDOW --");
        return;
    }

    /* Handle mouse events */
    if (key == KEY_MOUSE) {
        MouseEvent mouse;
        if (!terminal_read_mouse_event(&mouse)) return;

        Window *clicked_win = ed->active_window;

        if (mouse.button == 0 && mouse.press) {
            /* Left click - position cursor and start selection */
            if (clicked_win && clicked_win->content.buffer) {
                Buffer *buf = clicked_win->content.buffer;
                int file_row = mouse.y + clicked_win->row_offset - clicked_win->y;
                int file_col = mouse.x - clicked_win->x;

                /* Calculate gutter width */
                int gutter_width = 0;
                if (ed->show_line_numbers && buf->num_rows > 0) {
                    gutter_width = snprintf(NULL, 0, "%d", (int)buf->num_rows) + 1;
                }

                file_col -= gutter_width;
                if (file_col < 0) file_col = 0;

                if (file_row >= 0 && file_row < (int)buf->num_rows) {
                    buf->cursor_y = file_row;
                    BufferRow *row = &buf->rows[file_row];
                    buf->cursor_x = file_col < (int)row->size ? file_col : (int)row->size;
                    buf->has_selection = true;
                    buf->select_start_x = buf->cursor_x;
                    buf->select_start_y = buf->cursor_y;
                }
            }
        } else if (mouse.button == 0 && mouse.drag) {
            /* Left drag - extend selection */
            if (clicked_win && clicked_win->content.buffer) {
                Buffer *buf = clicked_win->content.buffer;
                int file_row = mouse.y + clicked_win->row_offset - clicked_win->y;
                int file_col = mouse.x - clicked_win->x;

                int gutter_width = 0;
                if (ed->show_line_numbers && buf->num_rows > 0) {
                    gutter_width = snprintf(NULL, 0, "%d", (int)buf->num_rows) + 1;
                }

                file_col -= gutter_width;
                if (file_col < 0) file_col = 0;

                if (file_row >= 0 && file_row < (int)buf->num_rows) {
                    buf->cursor_y = file_row;
                    BufferRow *row = &buf->rows[file_row];
                    buf->cursor_x = file_col < (int)row->size ? file_col : (int)row->size;
                }
            }
        } else if (mouse.button == 64) {
            /* Scroll up */
            if (clicked_win) {
                clicked_win->row_offset -= 3;
                if (clicked_win->row_offset < 0) clicked_win->row_offset = 0;
            }
        } else if (mouse.button == 65) {
            /* Scroll down */
            if (clicked_win && clicked_win->content.buffer) {
                clicked_win->row_offset += 3;
            }
        }
        return;
    }

    /* Check for custom keybindings first */
    if (ed->keymap && keymap_execute(ed->keymap, ed, key, KMOD_NONE) == 0) {
        return;  /* Keybinding handled it */
    }

    if (!ed->active_window || !ed->active_window->content.buffer) return;

    Buffer *buf = ed->active_window->content.buffer;

    switch (key) {
        case KEY_CTRL_Q:
            editor_quit(ed);
            break;

        case KEY_CTRL_S:
            if (buf->filename) {
                buffer_save(buf);
            }
            break;

        case KEY_ARROW_LEFT:
            if (buf->cursor_x > 0) {
                buf->cursor_x--;
            } else if (buf->cursor_y > 0) {
                buf->cursor_y--;
                if (buf->cursor_y < (int)buf->num_rows) {
                    buf->cursor_x = buf->rows[buf->cursor_y].size;
                }
            }
            break;

        case KEY_ARROW_RIGHT:
            if (buf->cursor_y < (int)buf->num_rows) {
                BufferRow *row = &buf->rows[buf->cursor_y];
                if (buf->cursor_x < (int)row->size) {
                    buf->cursor_x++;
                } else if (buf->cursor_y < (int)buf->num_rows - 1) {
                    buf->cursor_y++;
                    buf->cursor_x = 0;
                }
            }
            break;

        case KEY_ARROW_UP:
            if (buf->cursor_y > 0) {
                buf->cursor_y--;
                /* Adjust cursor_x if line is shorter */
                if (buf->cursor_y < (int)buf->num_rows) {
                    BufferRow *row = &buf->rows[buf->cursor_y];
                    if (buf->cursor_x > (int)row->size) {
                        buf->cursor_x = row->size;
                    }
                }
            }
            break;

        case KEY_ARROW_DOWN:
            if (buf->cursor_y < (int)buf->num_rows - 1) {
                buf->cursor_y++;
                /* Adjust cursor_x if line is shorter */
                if (buf->cursor_y < (int)buf->num_rows) {
                    BufferRow *row = &buf->rows[buf->cursor_y];
                    if (buf->cursor_x > (int)row->size) {
                        buf->cursor_x = row->size;
                    }
                }
            }
            break;

        case KEY_HOME:
            buf->cursor_x = 0;
            break;

        case KEY_END:
            if (buf->cursor_y < (int)buf->num_rows) {
                buf->cursor_x = buf->rows[buf->cursor_y].size;
            }
            break;

        case KEY_PAGE_UP:
            buf->cursor_y -= ed->term->rows;
            if (buf->cursor_y < 0) buf->cursor_y = 0;
            break;

        case KEY_PAGE_DOWN:
            buf->cursor_y += ed->term->rows;
            if (buf->cursor_y >= (int)buf->num_rows) {
                buf->cursor_y = buf->num_rows - 1;
            }
            if (buf->cursor_y < 0) buf->cursor_y = 0;
            break;

        case KEY_BACKSPACE:
            buffer_delete_char(buf);
            break;

        case KEY_ENTER:
            buffer_insert_newline(buf);
            break;

        case KEY_DEL:
            /* Move right then delete */
            if (buf->cursor_y < (int)buf->num_rows) {
                BufferRow *row = &buf->rows[buf->cursor_y];
                if (buf->cursor_x < (int)row->size) {
                    buf->cursor_x++;
                    buffer_delete_char(buf);
                }
            }
            break;

        case KEY_CTRL_Z:
            /* Undo */
            if (buf->undo_stack) {
                if (undo_apply(buf, buf->undo_stack) == 0) {
                    editor_set_status(ed, "Undo");
                } else {
                    editor_set_status(ed, "Nothing to undo");
                }
            }
            break;

        case KEY_CTRL_R:
            /* Redo */
            if (buf->undo_stack) {
                if (redo_apply(buf, buf->undo_stack) == 0) {
                    editor_set_status(ed, "Redo");
                } else {
                    editor_set_status(ed, "Nothing to redo");
                }
            }
            break;

        case KEY_CTRL_C:
            /* Copy selection to clipboard */
            if (buf->has_selection) {
                size_t len;
                char *text = buffer_get_selected_text(buf, &len);
                if (text) {
                    if (ed->clipboard) free(ed->clipboard);
                    ed->clipboard = text;
                    ed->clipboard_len = len;
                    editor_set_status(ed, "Copied");
                    buf->has_selection = false;
                }
            }
            break;

        case KEY_CTRL_V:
            /* Paste from clipboard */
            if (ed->clipboard && ed->clipboard_len > 0) {
                buffer_paste_text(buf, ed->clipboard, ed->clipboard_len);
                editor_set_status(ed, "Pasted");
            }
            break;

        default:
            /* Insert printable characters */
            if (key >= 32 && key < 127) {
                buffer_insert_char(buf, key);
            }
            break;
    }
}

static void editor_refresh_screen(Editor *ed) {
    terminal_clear(ed->term);

    /* Hide cursor during refresh */
    terminal_hide_cursor(ed->term);

    /* Clear screen */
    terminal_write_str(ed->term, "\x1b[2J");
    terminal_move_cursor(ed->term, 0, 0);

    /* Render windows */
    if (ed->root_window) {
        window_render(ed->root_window, ed->term, ed, ed->show_line_numbers);
    }

    /* Render status message at bottom */
    terminal_move_cursor(ed->term, ed->term->rows - 1, 0);
    terminal_write_str(ed->term, "\x1b[K"); /* Clear line */

    if (ed->status_len > 0) {
        /* Show status message */
        terminal_write(ed->term, ed->status_msg, ed->status_len);
    }

    /* Position cursor in buffer */
    if (ed->active_window && ed->active_window->content.buffer) {
        /* Cursor in buffer */
        Buffer *buf = ed->active_window->content.buffer;
        Window *win = ed->active_window;

        int screen_row = buf->cursor_y - win->row_offset;
        int screen_col = buf->cursor_x - win->col_offset;

        /* Calculate line number gutter width to offset cursor position */
        int gutter_width = 0;
        if (ed->show_line_numbers && buf->num_rows > 0) {
            int max_line = buf->num_rows;
            gutter_width = snprintf(NULL, 0, "%d", max_line) + 1; /* +1 for space */
            /* Add 2 more for git symbol + space */
            gutter_width += 2;
        }

        terminal_move_cursor(ed->term, win->y + screen_row, win->x + screen_col + gutter_width);
    }

    /* Show cursor */
    terminal_show_cursor(ed->term);

    terminal_flush(ed->term);
}

int editor_run(Editor *ed) {
    if (!ed) return -1;

    /* Enable raw mode */
    if (terminal_enable_raw_mode(ed->term) == -1) {
        return -1;
    }

    /* Enable mouse tracking */
    terminal_enable_mouse();

    /* Create initial buffer if none exists */
    if (ed->buffer_count == 0) {
        Buffer *buf = buffer_create();
        if (!buf) {
            terminal_disable_raw_mode(ed->term);
            return -1;
        }

        /* Add a single empty line */
        buffer_append_row(buf, "", 0);

        ed->buffers = malloc(sizeof(Buffer *));
        ed->buffers[0] = buf;
        ed->buffer_count = 1;

        /* Create window for buffer (leave room for command line) */
        ed->root_window = window_create_leaf(buf, 0, 0, ed->term->cols, ed->term->rows - 1);
        ed->active_window = ed->root_window;
    }

    /* Main loop */
    while (ed->running) {
        editor_refresh_screen(ed);

        int key = terminal_read_key();
        if (key != -1) {
            editor_process_keypress(ed, key);
        }

        /* Update window size (leave room for command line) */
        terminal_get_window_size(ed->term);
        if (ed->root_window) {
            window_resize(ed->root_window, 0, 0, ed->term->cols, ed->term->rows - 1);
        }
    }

    /* Cleanup */
    terminal_disable_mouse();
    terminal_disable_raw_mode(ed->term);

    /* Clear screen before exit */
    terminal_write_str(ed->term, "\x1b[2J");
    terminal_move_cursor(ed->term, 0, 0);
    terminal_flush(ed->term);

    return 0;
}
