#ifndef UNDO_H
#define UNDO_H

#include "buffer.h"
#include <stddef.h>

/* Undo action types */
typedef enum {
    UNDO_INSERT_CHAR,
    UNDO_DELETE_CHAR,
    UNDO_INSERT_LINE,
    UNDO_DELETE_LINE,
    UNDO_GROUP_BEGIN,
    UNDO_GROUP_END
} UndoActionType;

/* Undo action */
typedef struct UndoAction {
    UndoActionType type;
    int cursor_x;
    int cursor_y;

    union {
        struct {
            int c;
        } insert_char;

        struct {
            int c;
        } delete_char;

        struct {
            char *line_data;
            size_t line_len;
        } insert_line;

        struct {
            char *line_data;
            size_t line_len;
        } delete_line;
    } data;

    struct UndoAction *next;
    struct UndoAction *prev;
} UndoAction;

/* Undo stack */
typedef struct UndoStack {
    UndoAction *head;
    UndoAction *current;
    size_t size;
    size_t max_size;
} UndoStack;

/* Create/destroy undo stack */
UndoStack *undo_stack_create(size_t max_size);
void undo_stack_destroy(UndoStack *stack);

/* Push actions onto the stack */
void undo_push_insert_char(UndoStack *stack, int x, int y, int c);
void undo_push_delete_char(UndoStack *stack, int x, int y, int c);
void undo_push_insert_line(UndoStack *stack, int x, int y, const char *line, size_t len);
void undo_push_delete_line(UndoStack *stack, int x, int y, const char *line, size_t len);

/* Undo/redo operations */
int undo_apply(Buffer *buf, UndoStack *stack);
int redo_apply(Buffer *buf, UndoStack *stack);

/* Clear undo/redo */
void undo_stack_clear(UndoStack *stack);

#endif /* UNDO_H */
