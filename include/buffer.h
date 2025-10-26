#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdbool.h>

/* Forward declaration */
typedef struct Syntax Syntax;
typedef struct HighlightedLine HighlightedLine;
typedef struct UndoStack UndoStack;

/* Row in the buffer */
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} BufferRow;

/* Buffer structure - represents text content */
typedef struct Buffer {
    char *filename;
    BufferRow *rows;
    size_t num_rows;
    size_t capacity;
    bool modified;
    int cursor_x;
    int cursor_y;

    /* Syntax highlighting */
    Syntax *syntax;
    HighlightedLine **highlighted_lines;  /* Cached highlighting */
    bool *multiline_states;                /* Track multiline comment state */

    /* Undo/redo */
    UndoStack *undo_stack;

    /* Visual selection */
    bool has_selection;
    int select_start_x;
    int select_start_y;

    /* Search */
    char *search_term;  /* Current search term for highlighting */
} Buffer;

/* Position for bracket matching */
typedef struct {
    int row;
    int col;
    bool found;
} BracketMatch;

/* Buffer operations */
Buffer *buffer_create(void);
void buffer_destroy(Buffer *buf);
int buffer_open(Buffer *buf, const char *filename);
int buffer_save(Buffer *buf);
void buffer_insert_char(Buffer *buf, int c);
void buffer_insert_newline(Buffer *buf);
void buffer_delete_char(Buffer *buf);
void buffer_append_row(Buffer *buf, const char *s, size_t len);
void buffer_free_row(BufferRow *row);

/* Bracket matching */
BracketMatch buffer_find_matching_bracket(Buffer *buf);

/* Visual selection operations */
char *buffer_get_selected_text(Buffer *buf, size_t *len);
void buffer_delete_selection(Buffer *buf);
void buffer_paste_text(Buffer *buf, const char *text, size_t len);

#endif /* BUFFER_H */
