#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>
#include <stdbool.h>
#include <termios.h>

/* Terminal state */
typedef struct Terminal {
    int rows;
    int cols;
    struct termios orig_termios;
    char *screen_buffer;
    size_t buffer_size;
    size_t buffer_used;
} Terminal;

/* Key codes */
typedef enum {
    KEY_NULL = 0,
    KEY_CTRL_C = 3,
    KEY_CTRL_D = 4,
    KEY_CTRL_Q = 17,
    KEY_CTRL_R = 18,
    KEY_CTRL_S = 19,
    KEY_CTRL_V = 22,
    KEY_CTRL_W = 23,
    KEY_CTRL_Z = 26,
    KEY_ENTER = '\r',
    KEY_ESC = 27,
    KEY_BACKSPACE = 127,
    KEY_ARROW_LEFT = 1000,
    KEY_ARROW_RIGHT,
    KEY_ARROW_UP,
    KEY_ARROW_DOWN,
    KEY_DEL,
    KEY_HOME,
    KEY_END,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_MOUSE,
    KEY_CTRL_ARROW_LEFT,
    KEY_CTRL_ARROW_RIGHT,
    KEY_CTRL_ARROW_UP,
    KEY_CTRL_ARROW_DOWN,
} KeyCode;

/* Mouse event structure */
typedef struct {
    int x;
    int y;
    int button;  /* 0=left, 1=middle, 2=right, 64=scroll_up, 65=scroll_down */
    bool press;  /* true=press, false=release */
    bool drag;   /* true=drag event */
} MouseEvent;

/* Terminal functions */
Terminal *terminal_create(void);
void terminal_destroy(Terminal *term);
int terminal_enable_raw_mode(Terminal *term);
void terminal_disable_raw_mode(Terminal *term);
void terminal_enable_mouse(void);
void terminal_disable_mouse(void);
int terminal_get_window_size(Terminal *term);
int terminal_read_key(void);
bool terminal_read_mouse_event(MouseEvent *event);

/* Screen buffer functions */
void terminal_clear(Terminal *term);
void terminal_write(Terminal *term, const char *data, size_t len);
void terminal_write_str(Terminal *term, const char *str);
void terminal_flush(Terminal *term);
void terminal_move_cursor(Terminal *term, int row, int col);
void terminal_hide_cursor(Terminal *term);
void terminal_show_cursor(Terminal *term);

#endif /* TERMINAL_H */
