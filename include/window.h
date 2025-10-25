#ifndef WINDOW_H
#define WINDOW_H

#include "buffer.h"
#include <stddef.h>
#include <stdbool.h>

/* Forward declaration */
typedef struct Terminal Terminal;

/* Window types */
typedef enum {
    WINDOW_LEAF,   /* Contains a buffer */
    WINDOW_SPLIT_H, /* Horizontal split */
    WINDOW_SPLIT_V  /* Vertical split */
} WindowType;

/* Window structure - manages layout */
typedef struct Window {
    WindowType type;
    int x, y;           /* Position */
    int width, height;  /* Size */

    /* For leaf windows */
    Buffer *buffer;
    int row_offset;     /* Scroll offset */
    int col_offset;

    /* For split windows */
    struct Window *left;  /* or top */
    struct Window *right; /* or bottom */
    float split_ratio;
} Window;

/* Window operations */
Window *window_create_leaf(Buffer *buf, int x, int y, int w, int h);
Window *window_create_split(WindowType type, Window *left, Window *right, float ratio);
void window_destroy(Window *win);
void window_resize(Window *win, int x, int y, int w, int h);
void window_render(Window *win, Terminal *term, bool show_line_numbers);

/* Window navigation */
Window *window_find_leaf(Window *win, Window *target);
Window *window_get_next_leaf(Window *root, Window *current);
Window *window_get_prev_leaf(Window *root, Window *current);

/* Window management */
Window *window_close_split(Window *root, Window *to_close, Window **new_active);
Window *window_only(Window *root, Window *to_keep);

#endif /* WINDOW_H */
