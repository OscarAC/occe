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

    /* Initialize tab settings */
    ed->tab_width = 4;       /* Default to 4 spaces */
    ed->use_spaces = true;   /* Default to spaces instead of tabs */

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

/* Helper function to check if tab bar should be shown */
static bool editor_should_show_tabbar(Editor *ed) {
    if (!ed->tab_groups) return false;

    /* Count tabs */
    int tab_count = 0;
    TabGroup *tab = ed->tab_groups;
    while (tab) {
        tab_count++;
        tab = tab->next;
    }

    return tab_count > 1;
}

/* Helper function to get window layout dimensions */
static void editor_get_window_area(Editor *ed, int *y, int *height) {
    *y = editor_should_show_tabbar(ed) ? 1 : 0;  /* Start at row 1 if tab bar shown */
    *height = ed->term->rows - 1 - *y;  /* Leave room for status line and tab bar */
}

static void editor_process_keypress(Editor *ed, int key) {

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
            /* Delete character to the right, or join with next line if at end */
            if (buf->cursor_y < (int)buf->num_rows) {
                BufferRow *row = &buf->rows[buf->cursor_y];
                if (buf->cursor_x < (int)row->size) {
                    /* Delete character to the right */
                    buf->cursor_x++;
                    buffer_delete_char(buf);
                } else if (buf->cursor_y < (int)buf->num_rows - 1) {
                    /* At end of line - join with next line */
                    BufferRow *next_row = &buf->rows[buf->cursor_y + 1];

                    /* Ensure current row has space */
                    while (row->capacity < row->size + next_row->size + 1) {
                        row->capacity *= 2;
                        char *new_data = realloc(row->data, row->capacity);
                        if (!new_data) break;
                        row->data = new_data;
                    }

                    /* Append next row to current row */
                    memcpy(&row->data[row->size], next_row->data, next_row->size);
                    row->size += next_row->size;
                    row->data[row->size] = '\0';

                    /* Delete next row */
                    if (next_row->data) {
                        free(next_row->data);
                        next_row->data = NULL;
                    }

                    /* Shift rows up */
                    memmove(&buf->rows[buf->cursor_y + 1], &buf->rows[buf->cursor_y + 2],
                            sizeof(BufferRow) * (buf->num_rows - buf->cursor_y - 2));

                    buf->num_rows--;
                    buf->modified = true;
                }
            }
            break;

        case '\t':
            /* Tab key - insert tab or spaces based on configuration */
            if (ed->use_spaces) {
                /* Insert spaces */
                for (int i = 0; i < ed->tab_width; i++) {
                    buffer_insert_char(buf, ' ');
                }
            } else {
                /* Insert actual tab character */
                buffer_insert_char(buf, '\t');
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

/* Helper function to render the tab bar at the top of the screen */
static void editor_render_tabbar(Editor *ed) {
    if (!editor_should_show_tabbar(ed)) return;

    /* Move to top of screen */
    terminal_move_cursor(ed->term, 0, 0);

    /* Set background color for tab bar */
    terminal_write_str(ed->term, "\x1b[48;5;236m");  /* Dark gray background */

    int col = 0;
    int tab_index = 0;
    TabGroup *tab = ed->tab_groups;

    while (tab && col < ed->term->cols) {
        tab_index++;

        /* Determine if this is the active tab */
        bool is_active = (tab == ed->active_tab);

        /* Set colors for tab */
        if (is_active) {
            terminal_write_str(ed->term, "\x1b[1;38;5;15;48;5;24m");  /* Bold white on blue */
        } else {
            terminal_write_str(ed->term, "\x1b[0;38;5;250;48;5;236m"); /* Light gray on dark gray */
        }

        /* Format tab label */
        char tab_label[64];
        int label_len;

        /* Get a short name for the tab */
        const char *display_name = tab->name;
        if (display_name && strlen(display_name) > 20) {
            /* Truncate long names */
            snprintf(tab_label, sizeof(tab_label), " %d:%.17s... ", tab_index, display_name);
        } else if (display_name) {
            snprintf(tab_label, sizeof(tab_label), " %d:%s ", tab_index, display_name);
        } else {
            snprintf(tab_label, sizeof(tab_label), " %d ", tab_index);
        }

        label_len = strlen(tab_label);

        /* Don't render if it would overflow */
        if (col + label_len > ed->term->cols) break;

        /* Write the tab label */
        terminal_write(ed->term, tab_label, label_len);
        col += label_len;

        /* Add separator between tabs (not after active tab) */
        if (tab->next && !is_active && col < ed->term->cols) {
            terminal_write_str(ed->term, "\x1b[0;38;5;240;48;5;236m"); /* Dim separator */
            terminal_write_str(ed->term, "│");
            col++;
        }

        tab = tab->next;
    }

    /* Fill rest of line with background color */
    terminal_write_str(ed->term, "\x1b[0;48;5;236m");  /* Dark gray background */
    while (col < ed->term->cols) {
        terminal_write_str(ed->term, " ");
        col++;
    }

    /* Reset colors */
    terminal_write_str(ed->term, "\x1b[0m");
}

static void editor_refresh_screen(Editor *ed) {
    terminal_clear(ed->term);

    /* Hide cursor during refresh */
    terminal_hide_cursor(ed->term);

    /* Clear screen */
    terminal_write_str(ed->term, "\x1b[2J");
    terminal_move_cursor(ed->term, 0, 0);

    /* Render tab bar at top if we have multiple tabs */
    editor_render_tabbar(ed);

    /* Render windows */
    if (ed->root_window) {
        window_render(ed->root_window, ed->term, ed, ed->show_line_numbers);
    }

    /* Render status line at bottom */
    terminal_move_cursor(ed->term, ed->term->rows - 1, 0);
    terminal_write_str(ed->term, "\x1b[K"); /* Clear line */

    if (ed->status_len > 0) {
        /* Show status message */
        terminal_write(ed->term, ed->status_msg, ed->status_len);
    }

    /* Position cursor in buffer */
    if (ed->active_window && ed->active_window->content.buffer) {
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

        /* Create window for buffer (leave room for command line and tab bar) */
        int win_y, win_height;
        editor_get_window_area(ed, &win_y, &win_height);
        ed->root_window = window_create_leaf(buf, 0, win_y, ed->term->cols, win_height);
        ed->active_window = ed->root_window;
    }

    /* Main loop */
    while (ed->running) {
        editor_refresh_screen(ed);

        int key = terminal_read_key();
        if (key != -1) {
            editor_process_keypress(ed, key);
        }

        /* Update window size (leave room for command line and tab bar) */
        terminal_get_window_size(ed->term);
        if (ed->root_window) {
            int win_y, win_height;
            editor_get_window_area(ed, &win_y, &win_height);
            window_resize(ed->root_window, 0, win_y, ed->term->cols, win_height);
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
