#ifndef WINDOW_H
#define WINDOW_H

#include "buffer.h"
#include <stddef.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct Terminal Terminal;
typedef struct Editor Editor;

/* Window types */
typedef enum {
    WINDOW_LEAF,   /* Contains a buffer */
    WINDOW_SPLIT_H, /* Horizontal split */
    WINDOW_SPLIT_V  /* Vertical split */
} WindowType;

/* Window content type - determines rendering */
typedef enum {
    CONTENT_BUFFER,   /* Traditional buffer (default) */
    CONTENT_CUSTOM    /* Custom Lua renderer */
} ContentType;

/* Window structure - manages layout */
typedef struct Window {
    WindowType type;
    int x, y;           /* Position */
    int width, height;  /* Size */
    int id;             /* Unique window ID */

    /* For leaf windows */
    ContentType content_type;
    union {
        Buffer *buffer;         /* For CONTENT_BUFFER */
        void *custom_data;      /* For CONTENT_CUSTOM (opaque Lua data) */
    } content;
    char *renderer_name;        /* Lua function name for custom rendering */
    int row_offset;             /* Scroll offset */
    int col_offset;

    /* For split windows */
    struct Window *left;  /* or top */
    struct Window *right; /* or bottom */
    float split_ratio;

    /* Layout hints and metadata */
    int min_width, min_height;
    int max_width, max_height;
    float weight;               /* Proportional weight for resizing */
    bool focused;               /* Is this the active window? */
} Window;

/* Tab group - collection of window layouts */
typedef struct TabGroup {
    char *name;
    int id;
    Window *root_window;
    Window *active_window;
    struct TabGroup *next;
} TabGroup;

/* Window operations */
Window *window_create_leaf(Buffer *buf, int x, int y, int w, int h);
Window *window_create_leaf_custom(const char *renderer, int x, int y, int w, int h);
Window *window_create_split(WindowType type, Window *left, Window *right, float ratio);
void window_destroy(Window *win);
void window_resize(Window *win, int x, int y, int w, int h);
void window_render(Window *win, Terminal *term, Editor *ed, bool show_line_numbers);
Window *window_find_by_id(Window *root, int id);
void window_set_focused(Window *win, bool focused);

/* Window navigation */
Window *window_find_leaf(Window *win, Window *target);
Window *window_get_next_leaf(Window *root, Window *current);
Window *window_get_prev_leaf(Window *root, Window *current);
Window *window_get_direction(Window *root, Window *current, const char *direction);

/* Window management */
Window *window_close_split(Window *root, Window *to_close, Window **new_active);
Window *window_only(Window *root, Window *to_keep);
void window_swap(Window *a, Window *b);
void window_set_split_ratio(Window *win, float ratio);

/* Advanced layout operations */
void window_equalize_sizes(Window *win);
int window_count_leaves(Window *win);
void window_collect_all_leaves(Window *win, Window **leaves, int *count, int max);
Window *window_find_parent(Window *root, Window *child);
Window *window_find_sibling(Window *root, Window *target);

/* Tab group operations */
TabGroup *tabgroup_create(const char *name, Window *root);
void tabgroup_destroy(TabGroup *group);
TabGroup *tabgroup_find_by_id(TabGroup *head, int id);

#endif /* WINDOW_H */
