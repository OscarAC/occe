#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#define INITIAL_BUFFER_SIZE 4096

Terminal *terminal_create(void) {
    Terminal *term = malloc(sizeof(Terminal));
    if (!term) return NULL;

    term->rows = 24;
    term->cols = 80;
    term->buffer_size = INITIAL_BUFFER_SIZE;
    term->buffer_used = 0;
    term->screen_buffer = malloc(term->buffer_size);

    if (!term->screen_buffer) {
        free(term);
        return NULL;
    }

    if (terminal_get_window_size(term) == -1) {
        /* Failed to get size, use defaults */
    }

    return term;
}

void terminal_destroy(Terminal *term) {
    if (!term) return;

    if (term->screen_buffer) {
        free(term->screen_buffer);
    }
    free(term);
}

int terminal_enable_raw_mode(Terminal *term) {
    if (tcgetattr(STDIN_FILENO, &term->orig_termios) == -1) {
        return -1;
    }

    struct termios raw = term->orig_termios;

    /* Disable canonical mode, echo, signals, and special processing */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    /* Set read timeout */
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        return -1;
    }

    return 0;
}

void terminal_disable_raw_mode(Terminal *term) {
    if (term) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &term->orig_termios);
    }
}

void terminal_enable_mouse(void) {
    /* Enable mouse tracking:
     * ?1000h - Normal mouse tracking (clicks)
     * ?1002h - Button event tracking (drag)
     * ?1006h - SGR extended mouse mode (better coordinates)
     */
    const char *seq = "\x1b[?1000h\x1b[?1002h\x1b[?1006h";
    write(STDOUT_FILENO, seq, strlen(seq));
}

void terminal_disable_mouse(void) {
    /* Disable mouse tracking */
    const char *seq = "\x1b[?1000l\x1b[?1002l\x1b[?1006l";
    write(STDOUT_FILENO, seq, strlen(seq));
}

int terminal_get_window_size(Terminal *term) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        /* Fallback method using cursor positioning */
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return -1;  /* Would need to read cursor position */
    }

    term->cols = ws.ws_col;
    term->rows = ws.ws_row;
    return 0;
}

/* Static storage for last mouse event */
static MouseEvent last_mouse_event = {0};
static bool has_mouse_event = false;

bool terminal_read_mouse_event(MouseEvent *event) {
    if (!event || !has_mouse_event) return false;

    *event = last_mouse_event;
    has_mouse_event = false;
    return true;
}

/* Helper to parse SGR mouse events */
static bool parse_mouse_sgr(MouseEvent *event) {
    char buf[32];
    int idx = 0;

    /* Read until 'M' (press) or 'm' (release) */
    while (idx < 31) {
        if (read(STDIN_FILENO, &buf[idx], 1) != 1) return false;
        if (buf[idx] == 'M' || buf[idx] == 'm') {
            buf[idx + 1] = '\0';
            break;
        }
        idx++;
    }

    if (idx >= 31) return false;

    bool press = (buf[idx] == 'M');
    buf[idx] = '\0';

    /* Parse: B;X;Y format */
    int button, x, y;
    if (sscanf(buf, "%d;%d;%d", &button, &x, &y) != 3) return false;

    /* Store event */
    event->button = button;
    event->x = x - 1;  /* Convert to 0-based */
    event->y = y - 1;
    event->press = press;
    event->drag = (button & 32) != 0;  /* Bit 32 indicates drag */

    return true;
}

int terminal_read_key(void) {
    int nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) return -1;
    }

    if (c == '\x1b') {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            /* Check for SGR mouse event: \x1b[< */
            if (seq[1] == '<') {
                if (parse_mouse_sgr(&last_mouse_event)) {
                    has_mouse_event = true;
                    return KEY_MOUSE;
                }
                return '\x1b';
            }

            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return KEY_HOME;
                        case '3': return KEY_DEL;
                        case '4': return KEY_END;
                        case '5': return KEY_PAGE_UP;
                        case '6': return KEY_PAGE_DOWN;
                        case '7': return KEY_HOME;
                        case '8': return KEY_END;
                    }
                } else if (seq[1] == '1' && seq[2] == ';') {
                    /* Read modifier and direction: \x1b[1;5X */
                    char mod, dir;
                    if (read(STDIN_FILENO, &mod, 1) != 1) return '\x1b';
                    if (read(STDIN_FILENO, &dir, 1) != 1) return '\x1b';

                    if (mod == '5') {  /* Ctrl modifier */
                        switch (dir) {
                            case 'A':
                                return KEY_CTRL_ARROW_UP;
                            case 'B':
                                return KEY_CTRL_ARROW_DOWN;
                            case 'C':
                                return KEY_CTRL_ARROW_RIGHT;
                            case 'D':
                                return KEY_CTRL_ARROW_LEFT;
                        }
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return KEY_ARROW_UP;
                    case 'B': return KEY_ARROW_DOWN;
                    case 'C': return KEY_ARROW_RIGHT;
                    case 'D': return KEY_ARROW_LEFT;
                    case 'H': return KEY_HOME;
                    case 'F': return KEY_END;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return KEY_HOME;
                case 'F': return KEY_END;
            }
        }

        return '\x1b';
    }

    return c;
}

void terminal_clear(Terminal *term) {
    term->buffer_used = 0;
}

void terminal_write(Terminal *term, const char *data, size_t len) {
    while (term->buffer_used + len > term->buffer_size) {
        size_t new_size = term->buffer_size * 2;
        char *new_buf = realloc(term->screen_buffer, new_size);
        if (!new_buf) return; /* Allocation failed, data lost */
        term->screen_buffer = new_buf;
        term->buffer_size = new_size;
    }

    memcpy(term->screen_buffer + term->buffer_used, data, len);
    term->buffer_used += len;
}

void terminal_write_str(Terminal *term, const char *str) {
    terminal_write(term, str, strlen(str));
}

void terminal_flush(Terminal *term) {
    if (term->buffer_used > 0) {
        write(STDOUT_FILENO, term->screen_buffer, term->buffer_used);
        term->buffer_used = 0;
    }
}

void terminal_move_cursor(Terminal *term, int row, int col) {
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row + 1, col + 1);
    terminal_write_str(term, buf);
}

void terminal_hide_cursor(Terminal *term) {
    terminal_write_str(term, "\x1b[?25l");
}

void terminal_show_cursor(Terminal *term) {
    terminal_write_str(term, "\x1b[?25h");
}

