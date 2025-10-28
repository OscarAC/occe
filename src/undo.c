#include "undo.h"
#include "buffer.h"
#include <stdlib.h>
#include <string.h>

UndoStack *undo_stack_create(size_t max_size) {
    UndoStack *stack = malloc(sizeof(UndoStack));
    if (!stack) return NULL;

    stack->head = NULL;
    stack->current = NULL;
    stack->size = 0;
    stack->max_size = max_size;

    return stack;
}

static void undo_action_destroy(UndoAction *action) {
    if (!action) return;

    /* Free line data if present */
    if (action->type == UNDO_INSERT_LINE && action->data.insert_line.line_data) {
        free(action->data.insert_line.line_data);
    } else if (action->type == UNDO_DELETE_LINE && action->data.delete_line.line_data) {
        free(action->data.delete_line.line_data);
    }

    free(action);
}

void undo_stack_destroy(UndoStack *stack) {
    if (!stack) return;

    UndoAction *action = stack->head;
    while (action) {
        UndoAction *next = action->next;
        undo_action_destroy(action);
        action = next;
    }

    free(stack);
}

static void undo_clear_redo(UndoStack *stack) {
    if (!stack || !stack->current) return;

    /* Clear all actions after current */
    UndoAction *action = stack->current->next;
    while (action) {
        UndoAction *next = action->next;
        undo_action_destroy(action);
        stack->size--;
        action = next;
    }

    if (stack->current) {
        stack->current->next = NULL;
    }
}

static void undo_push_action(UndoStack *stack, UndoAction *action) {
    if (!stack || !action) return;

    /* Clear redo history when pushing new action */
    undo_clear_redo(stack);

    /* Link action into list */
    action->prev = stack->current;
    action->next = NULL;

    if (stack->current) {
        stack->current->next = action;
    } else {
        stack->head = action;
    }

    stack->current = action;
    stack->size++;

    /* Enforce max size by removing oldest actions */
    while (stack->size > stack->max_size && stack->head) {
        UndoAction *old_head = stack->head;
        stack->head = old_head->next;
        if (stack->head) {
            stack->head->prev = NULL;
        }
        undo_action_destroy(old_head);
        stack->size--;
    }
}

void undo_push_insert_char(UndoStack *stack, int x, int y, int c) {
    UndoAction *action = malloc(sizeof(UndoAction));
    if (!action) return;

    action->type = UNDO_INSERT_CHAR;
    action->cursor_x = x;
    action->cursor_y = y;
    action->data.insert_char.c = c;

    undo_push_action(stack, action);
}

void undo_push_delete_char(UndoStack *stack, int x, int y, int c) {
    UndoAction *action = malloc(sizeof(UndoAction));
    if (!action) return;

    action->type = UNDO_DELETE_CHAR;
    action->cursor_x = x;
    action->cursor_y = y;
    action->data.delete_char.c = c;

    undo_push_action(stack, action);
}

void undo_push_insert_line(UndoStack *stack, int x, int y, const char *line, size_t len) {
    UndoAction *action = malloc(sizeof(UndoAction));
    if (!action) return;

    action->type = UNDO_INSERT_LINE;
    action->cursor_x = x;
    action->cursor_y = y;

    /* Copy line data */
    action->data.insert_line.line_data = malloc(len + 1);
    if (!action->data.insert_line.line_data) {
        free(action);
        return;
    }
    memcpy(action->data.insert_line.line_data, line, len);
    action->data.insert_line.line_data[len] = '\0';
    action->data.insert_line.line_len = len;

    undo_push_action(stack, action);
}

void undo_push_delete_line(UndoStack *stack, int x, int y, const char *line, size_t len) {
    UndoAction *action = malloc(sizeof(UndoAction));
    if (!action) return;

    action->type = UNDO_DELETE_LINE;
    action->cursor_x = x;
    action->cursor_y = y;

    /* Copy line data */
    action->data.delete_line.line_data = malloc(len + 1);
    if (!action->data.delete_line.line_data) {
        free(action);
        return;
    }
    memcpy(action->data.delete_line.line_data, line, len);
    action->data.delete_line.line_data[len] = '\0';
    action->data.delete_line.line_len = len;

    undo_push_action(stack, action);
}

/* Forward declarations for buffer operations without undo recording */
static void buffer_insert_char_raw(Buffer *buf, int c);
static void buffer_delete_char_raw(Buffer *buf, int at_x, int at_y);

int undo_apply(Buffer *buf, UndoStack *stack) {
    if (!buf || !stack || !stack->current) return -1;

    UndoAction *action = stack->current;

    /* Move cursor to action position */
    buf->cursor_x = action->cursor_x;
    buf->cursor_y = action->cursor_y;

    switch (action->type) {
        case UNDO_INSERT_CHAR:
            /* Undo insert by deleting */
            buffer_delete_char_raw(buf, action->cursor_x, action->cursor_y);
            buf->cursor_x = action->cursor_x;
            buf->cursor_y = action->cursor_y;
            break;

        case UNDO_DELETE_CHAR:
            /* Undo delete by inserting */
            buffer_insert_char_raw(buf, action->data.delete_char.c);
            buf->cursor_x = action->cursor_x;
            buf->cursor_y = action->cursor_y;
            break;

        case UNDO_INSERT_LINE:
            /* Undo insert line by deleting line */
            /* TODO: Implement */
            break;

        case UNDO_DELETE_LINE:
            /* Undo delete line by inserting line */
            /* TODO: Implement */
            break;

        default:
            break;
    }

    /* Move to previous action */
    stack->current = action->prev;

    return 0;
}

int redo_apply(Buffer *buf, UndoStack *stack) {
    if (!buf || !stack) return -1;

    UndoAction *action;
    if (stack->current) {
        action = stack->current->next;
    } else {
        action = stack->head;
    }

    if (!action) return -1;  /* Nothing to redo */

    /* Move cursor to action position */
    buf->cursor_x = action->cursor_x;
    buf->cursor_y = action->cursor_y;

    switch (action->type) {
        case UNDO_INSERT_CHAR:
            /* Redo insert */
            buffer_insert_char_raw(buf, action->data.insert_char.c);
            break;

        case UNDO_DELETE_CHAR:
            /* Redo delete */
            buffer_delete_char_raw(buf, action->cursor_x, action->cursor_y);
            break;

        case UNDO_INSERT_LINE:
            /* Redo insert line */
            /* TODO: Implement */
            break;

        case UNDO_DELETE_LINE:
            /* Redo delete line */
            /* TODO: Implement */
            break;

        default:
            break;
    }

    /* Move current forward */
    stack->current = action;

    return 0;
}

void undo_stack_clear(UndoStack *stack) {
    if (!stack) return;

    UndoAction *action = stack->head;
    while (action) {
        UndoAction *next = action->next;
        undo_action_destroy(action);
        action = next;
    }

    stack->head = NULL;
    stack->current = NULL;
    stack->size = 0;
}

/* Raw buffer operations that don't record undo */
static void buffer_insert_char_raw(Buffer *buf, int c) {
    if (!buf) return;

    if (buf->cursor_y >= (int)buf->num_rows) {
        /* Create new row if needed */
        buffer_append_row(buf, "", 0);
    }

    BufferRow *row = &buf->rows[buf->cursor_y];

    /* Ensure capacity */
    if (row->size + 1 >= row->capacity) {
        row->capacity = row->capacity == 0 ? 128 : row->capacity * 2;
        char *new_data = realloc(row->data, row->capacity);
        if (!new_data) return;
        row->data = new_data;
    }

    /* Insert character */
    memmove(&row->data[buf->cursor_x + 1], &row->data[buf->cursor_x],
            row->size - buf->cursor_x);
    row->data[buf->cursor_x] = c;
    row->size++;
    row->data[row->size] = '\0';

    buf->cursor_x++;
    buf->modified = true;
}

static void buffer_delete_char_raw(Buffer *buf, int at_x, int at_y) {
    if (!buf || at_y >= (int)buf->num_rows) return;

    BufferRow *row = &buf->rows[at_y];
    if (at_x >= (int)row->size) return;

    /* Delete character */
    memmove(&row->data[at_x], &row->data[at_x + 1], row->size - at_x);
    row->size--;
    row->data[row->size] = '\0';

    buf->modified = true;
}

