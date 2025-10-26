#define _POSIX_C_SOURCE 200809L
#include "buffer.h"
#include "syntax.h"
#include "undo.h"
#include "git.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_ROW_CAPACITY 16

Buffer *buffer_create(void) {
    Buffer *buf = malloc(sizeof(Buffer));
    if (!buf) return NULL;

    buf->filename = NULL;
    buf->rows = NULL;
    buf->num_rows = 0;
    buf->capacity = 0;
    buf->modified = false;
    buf->cursor_x = 0;
    buf->cursor_y = 0;

    /* Syntax highlighting */
    buf->syntax = NULL;
    buf->highlighted_lines = NULL;
    buf->multiline_states = NULL;

    /* Undo/redo */
    buf->undo_stack = undo_stack_create(1000);  /* Max 1000 undo levels */

    /* Visual selection */
    buf->has_selection = false;
    buf->select_start_x = 0;
    buf->select_start_y = 0;

    /* Git integration */
    buf->git_diff = NULL;
    buf->git_branch = NULL;

    /* Search */
    buf->search_term = NULL;

    return buf;
}

void buffer_free_row(BufferRow *row) {
    if (row && row->data) {
        free(row->data);
        row->data = NULL;
        row->size = 0;
        row->capacity = 0;
    }
}

/* Helper to invalidate highlighting cache from a given row onwards */
static void buffer_invalidate_highlighting(Buffer *buf, size_t from_row) {
    if (!buf->highlighted_lines) return;

    /* Free cached highlighting from from_row onwards */
    for (size_t i = from_row; i < buf->num_rows; i++) {
        if (buf->highlighted_lines[i]) {
            syntax_free_highlighted_line(buf->highlighted_lines[i]);
            buf->highlighted_lines[i] = NULL;
        }
    }
}

/* Helper to reallocate highlighting cache when buffer size changes */
static void buffer_resize_highlighting_cache(Buffer *buf, size_t old_rows, size_t new_rows) {
    if (!buf->syntax) return;

    /* Free all cached highlighting first */
    if (buf->highlighted_lines) {
        for (size_t i = 0; i < old_rows; i++) {
            if (buf->highlighted_lines[i]) {
                syntax_free_highlighted_line(buf->highlighted_lines[i]);
                buf->highlighted_lines[i] = NULL;
            }
        }
        free(buf->highlighted_lines);
        free(buf->multiline_states);
    }

    /* Reallocate for new size */
    if (new_rows > 0) {
        buf->highlighted_lines = calloc(new_rows, sizeof(HighlightedLine *));
        buf->multiline_states = calloc(new_rows, sizeof(bool));
    } else {
        buf->highlighted_lines = NULL;
        buf->multiline_states = NULL;
    }
}

void buffer_destroy(Buffer *buf) {
    if (!buf) return;

    for (size_t i = 0; i < buf->num_rows; i++) {
        buffer_free_row(&buf->rows[i]);
    }

    /* Free highlighting cache */
    if (buf->highlighted_lines) {
        for (size_t i = 0; i < buf->num_rows; i++) {
            if (buf->highlighted_lines[i]) {
                syntax_free_highlighted_line(buf->highlighted_lines[i]);
            }
        }
        free(buf->highlighted_lines);
    }
    if (buf->multiline_states) free(buf->multiline_states);

    if (buf->rows) free(buf->rows);
    if (buf->filename) free(buf->filename);
    if (buf->undo_stack) undo_stack_destroy(buf->undo_stack);
    if (buf->git_diff) git_diff_free(buf->git_diff);
    if (buf->git_branch) free(buf->git_branch);
    if (buf->search_term) free(buf->search_term);
    free(buf);
}

void buffer_append_row(Buffer *buf, const char *s, size_t len) {
    if (buf->num_rows >= buf->capacity) {
        size_t new_capacity = buf->capacity == 0 ? INITIAL_ROW_CAPACITY : buf->capacity * 2;
        BufferRow *new_rows = realloc(buf->rows, sizeof(BufferRow) * new_capacity);
        if (!new_rows) return;
        buf->rows = new_rows;
        buf->capacity = new_capacity;
    }

    BufferRow *row = &buf->rows[buf->num_rows];
    row->capacity = len + 1;
    row->data = malloc(row->capacity);
    if (!row->data) return;

    memcpy(row->data, s, len);
    row->data[len] = '\0';
    row->size = len;

    buf->num_rows++;
    buf->modified = true;
}

int buffer_open(Buffer *buf, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return -1;

    buf->filename = strdup(filename);

    /* Detect syntax based on filename */
    buf->syntax = syntax_find_by_filename(filename);

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        /* Remove trailing newline/carriage return */
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
            linelen--;
        }
        buffer_append_row(buf, line, linelen);
    }

    free(line);
    fclose(fp);
    buf->modified = false;

    /* Initialize highlighting cache */
    if (buf->syntax && buf->num_rows > 0) {
        buf->highlighted_lines = calloc(buf->num_rows, sizeof(HighlightedLine *));
        buf->multiline_states = calloc(buf->num_rows, sizeof(bool));
    }

    return 0;
}

int buffer_save(Buffer *buf) {
    if (!buf->filename) return -1;

    FILE *fp = fopen(buf->filename, "w");
    if (!fp) return -1;

    for (size_t i = 0; i < buf->num_rows; i++) {
        fwrite(buf->rows[i].data, 1, buf->rows[i].size, fp);
        fputc('\n', fp);
    }

    fclose(fp);
    buf->modified = false;
    return 0;
}

void buffer_insert_char(Buffer *buf, int c) {
    if (buf->cursor_y == (int)buf->num_rows) {
        /* Insert at end of file */
        buffer_append_row(buf, "", 0);
    }

    if (buf->cursor_y >= (int)buf->num_rows) return;

    BufferRow *row = &buf->rows[buf->cursor_y];

    /* Ensure we have space */
    if (row->size + 1 >= row->capacity) {
        row->capacity = row->capacity == 0 ? 16 : row->capacity * 2;
        char *new_data = realloc(row->data, row->capacity);
        if (!new_data) return;
        row->data = new_data;
    }

    /* Record undo before modifying */
    if (buf->undo_stack) {
        undo_push_insert_char(buf->undo_stack, buf->cursor_x, buf->cursor_y, c);
    }

    /* Insert character */
    memmove(&row->data[buf->cursor_x + 1], &row->data[buf->cursor_x],
            row->size - buf->cursor_x);
    row->data[buf->cursor_x] = c;
    row->size++;
    row->data[row->size] = '\0';

    buf->cursor_x++;
    buf->modified = true;

    /* Invalidate highlighting for this line and onwards (for multiline comments) */
    buffer_invalidate_highlighting(buf, buf->cursor_y);
}

void buffer_insert_newline(Buffer *buf) {
    if (buf->cursor_y >= (int)buf->num_rows) {
        size_t old_rows = buf->num_rows;
        buffer_append_row(buf, "", 0);
        /* Resize highlighting cache to accommodate new row */
        buffer_resize_highlighting_cache(buf, old_rows, buf->num_rows);
        buf->cursor_y++;
        buf->cursor_x = 0;
        return;
    }

    BufferRow *row = &buf->rows[buf->cursor_y];

    /* Split the current row at cursor */
    const char *rest = (buf->cursor_x < (int)row->size) ? &row->data[buf->cursor_x] : "";
    size_t rest_len = (buf->cursor_x < (int)row->size) ? row->size - buf->cursor_x : 0;

    /* Truncate current row */
    row->size = buf->cursor_x;
    row->data[row->size] = '\0';

    /* Make space for new row */
    if (buf->num_rows >= buf->capacity) {
        size_t new_capacity = buf->capacity == 0 ? INITIAL_ROW_CAPACITY : buf->capacity * 2;
        BufferRow *new_rows = realloc(buf->rows, sizeof(BufferRow) * new_capacity);
        if (!new_rows) return;
        buf->rows = new_rows;
        buf->capacity = new_capacity;
    }

    /* Shift rows down */
    memmove(&buf->rows[buf->cursor_y + 2], &buf->rows[buf->cursor_y + 1],
            sizeof(BufferRow) * (buf->num_rows - buf->cursor_y - 1));

    /* Insert new row */
    BufferRow *new_row = &buf->rows[buf->cursor_y + 1];
    new_row->capacity = rest_len + 1;
    new_row->data = malloc(new_row->capacity);
    if (!new_row->data) return;

    memcpy(new_row->data, rest, rest_len);
    new_row->data[rest_len] = '\0';
    new_row->size = rest_len;

    size_t old_rows = buf->num_rows;
    buf->num_rows++;

    /* Resize highlighting cache to accommodate new row */
    buffer_resize_highlighting_cache(buf, old_rows, buf->num_rows);

    buf->cursor_y++;

    /* Auto-indent: copy leading whitespace from previous line */
    BufferRow *prev_row = &buf->rows[buf->cursor_y - 1];
    int indent = 0;
    while (indent < (int)prev_row->size &&
           (prev_row->data[indent] == ' ' || prev_row->data[indent] == '\t')) {
        indent++;
    }

    if (indent > 0) {
        /* Ensure new row has enough capacity */
        while (new_row->capacity < new_row->size + indent + 1) {
            new_row->capacity *= 2;
            char *new_data = realloc(new_row->data, new_row->capacity);
            if (!new_data) break;
            new_row->data = new_data;
        }

        /* Shift existing content right */
        memmove(&new_row->data[indent], new_row->data, new_row->size);

        /* Copy indent */
        memcpy(new_row->data, prev_row->data, indent);
        new_row->size += indent;
        new_row->data[new_row->size] = '\0';

        buf->cursor_x = indent;
    } else {
        buf->cursor_x = 0;
    }

    buf->modified = true;
}

void buffer_delete_char(Buffer *buf) {
    if (buf->cursor_y >= (int)buf->num_rows) return;
    if (buf->cursor_x == 0 && buf->cursor_y == 0) return;

    BufferRow *row = &buf->rows[buf->cursor_y];

    if (buf->cursor_x > 0) {
        /* Record undo before deleting */
        if (buf->undo_stack) {
            char deleted_char = row->data[buf->cursor_x - 1];
            undo_push_delete_char(buf->undo_stack, buf->cursor_x - 1, buf->cursor_y, deleted_char);
        }

        /* Delete character before cursor */
        memmove(&row->data[buf->cursor_x - 1], &row->data[buf->cursor_x],
                row->size - buf->cursor_x);
        row->size--;
        row->data[row->size] = '\0';
        buf->cursor_x--;
        buf->modified = true;

        /* Invalidate highlighting for this line and onwards (for multiline comments) */
        buffer_invalidate_highlighting(buf, buf->cursor_y);
    } else {
        /* Delete newline - join with previous row */
        BufferRow *prev_row = &buf->rows[buf->cursor_y - 1];
        buf->cursor_x = prev_row->size;

        /* Ensure previous row has space */
        while (prev_row->capacity < prev_row->size + row->size + 1) {
            prev_row->capacity *= 2;
            char *new_data = realloc(prev_row->data, prev_row->capacity);
            if (!new_data) return;
            prev_row->data = new_data;
        }

        /* Append current row to previous */
        memcpy(&prev_row->data[prev_row->size], row->data, row->size);
        prev_row->size += row->size;
        prev_row->data[prev_row->size] = '\0';

        /* Delete current row */
        buffer_free_row(row);

        /* Shift rows up */
        memmove(&buf->rows[buf->cursor_y], &buf->rows[buf->cursor_y + 1],
                sizeof(BufferRow) * (buf->num_rows - buf->cursor_y - 1));

        size_t old_rows = buf->num_rows;
        buf->num_rows--;

        /* Resize highlighting cache after deleting row */
        buffer_resize_highlighting_cache(buf, old_rows, buf->num_rows);

        buf->cursor_y--;
        buf->modified = true;
    }
}

BracketMatch buffer_find_matching_bracket(Buffer *buf) {
    BracketMatch result = {-1, -1, false};

    if (!buf || buf->cursor_y >= (int)buf->num_rows) {
        return result;
    }

    BufferRow *row = &buf->rows[buf->cursor_y];
    if (buf->cursor_x >= (int)row->size) {
        return result;
    }

    char c = row->data[buf->cursor_x];
    char match_char;
    int direction;  /* 1 for forward, -1 for backward */
    bool is_opening;

    /* Determine bracket type and direction */
    switch (c) {
        case '(': match_char = ')'; direction = 1; is_opening = true; break;
        case ')': match_char = '('; direction = -1; is_opening = false; break;
        case '{': match_char = '}'; direction = 1; is_opening = true; break;
        case '}': match_char = '{'; direction = -1; is_opening = false; break;
        case '[': match_char = ']'; direction = 1; is_opening = true; break;
        case ']': match_char = '['; direction = -1; is_opening = false; break;
        default:
            return result;  /* Not on a bracket */
    }

    int count = 1;  /* Count of unmatched brackets */
    int curr_row = buf->cursor_y;
    int curr_col = buf->cursor_x + direction;

    /* Search for matching bracket */
    while (curr_row >= 0 && curr_row < (int)buf->num_rows) {
        BufferRow *search_row = &buf->rows[curr_row];

        if (direction == 1) {
            /* Search forward */
            while (curr_col < (int)search_row->size) {
                char ch = search_row->data[curr_col];
                if (ch == c) {
                    count++;
                } else if (ch == match_char) {
                    count--;
                    if (count == 0) {
                        result.row = curr_row;
                        result.col = curr_col;
                        result.found = true;
                        return result;
                    }
                }
                curr_col++;
            }
            curr_row++;
            curr_col = 0;
        } else {
            /* Search backward */
            while (curr_col >= 0) {
                char ch = search_row->data[curr_col];
                if (ch == c) {
                    count++;
                } else if (ch == match_char) {
                    count--;
                    if (count == 0) {
                        result.row = curr_row;
                        result.col = curr_col;
                        result.found = true;
                        return result;
                    }
                }
                curr_col--;
            }
            curr_row--;
            if (curr_row >= 0) {
                curr_col = buf->rows[curr_row].size - 1;
            }
        }
    }

    return result;  /* No match found */
}

char *buffer_get_selected_text(Buffer *buf, size_t *len) {
    if (!buf || !buf->has_selection) {
        *len = 0;
        return NULL;
    }

    /* Normalize selection (start <= end) */
    int start_y = buf->select_start_y;
    int start_x = buf->select_start_x;
    int end_y = buf->cursor_y;
    int end_x = buf->cursor_x;

    if (start_y > end_y || (start_y == end_y && start_x > end_x)) {
        /* Swap */
        int tmp = start_y; start_y = end_y; end_y = tmp;
        tmp = start_x; start_x = end_x; end_x = tmp;
    }

    /* Calculate total size needed */
    size_t total_size = 0;
    for (int y = start_y; y <= end_y && y < (int)buf->num_rows; y++) {
        BufferRow *row = &buf->rows[y];
        if (y == start_y && y == end_y) {
            /* Single line selection */
            int count = end_x - start_x;
            if (count > 0 && start_x < (int)row->size) {
                total_size += count;
            }
        } else if (y == start_y) {
            /* First line of multi-line */
            total_size += row->size - start_x + 1; /* +1 for newline */
        } else if (y == end_y) {
            /* Last line of multi-line */
            total_size += end_x;
        } else {
            /* Middle lines */
            total_size += row->size + 1; /* +1 for newline */
        }
    }

    if (total_size == 0) {
        *len = 0;
        return NULL;
    }

    /* Allocate buffer */
    char *text = malloc(total_size + 1);
    if (!text) {
        *len = 0;
        return NULL;
    }

    /* Copy selected text */
    size_t pos = 0;
    for (int y = start_y; y <= end_y && y < (int)buf->num_rows; y++) {
        BufferRow *row = &buf->rows[y];
        if (y == start_y && y == end_y) {
            /* Single line selection */
            int count = end_x - start_x;
            if (count > 0 && start_x < (int)row->size) {
                int copy_len = count;
                if (start_x + copy_len > (int)row->size) {
                    copy_len = row->size - start_x;
                }
                memcpy(text + pos, row->data + start_x, copy_len);
                pos += copy_len;
            }
        } else if (y == start_y) {
            /* First line */
            int copy_len = row->size - start_x;
            if (copy_len > 0) {
                memcpy(text + pos, row->data + start_x, copy_len);
                pos += copy_len;
            }
            text[pos++] = '\n';
        } else if (y == end_y) {
            /* Last line */
            int copy_len = end_x;
            if (copy_len > (int)row->size) copy_len = row->size;
            if (copy_len > 0) {
                memcpy(text + pos, row->data, copy_len);
                pos += copy_len;
            }
        } else {
            /* Middle lines */
            memcpy(text + pos, row->data, row->size);
            pos += row->size;
            text[pos++] = '\n';
        }
    }

    text[pos] = '\0';
    *len = pos;
    return text;
}

void buffer_delete_selection(Buffer *buf) {
    if (!buf || !buf->has_selection) return;

    /* Normalize selection */
    int start_y = buf->select_start_y;
    int start_x = buf->select_start_x;
    int end_y = buf->cursor_y;
    int end_x = buf->cursor_x;

    if (start_y > end_y || (start_y == end_y && start_x > end_x)) {
        int tmp = start_y; start_y = end_y; end_y = tmp;
        tmp = start_x; start_x = end_x; end_x = tmp;
    }

    /* Delete selection */
    if (start_y == end_y) {
        /* Single line deletion */
        if (start_y >= (int)buf->num_rows) return;
        BufferRow *row = &buf->rows[start_y];

        /* Clamp end_x to row size to prevent out of bounds access */
        if (end_x > (int)row->size) end_x = row->size;

        int delete_count = end_x - start_x;
        if (delete_count > 0 && start_x < (int)row->size) {
            memmove(&row->data[start_x], &row->data[end_x],
                    row->size - end_x);
            row->size -= delete_count;
            row->data[row->size] = '\0';
        }

        buf->cursor_x = start_x;
        buf->cursor_y = start_y;

        /* Invalidate highlighting for this line and onwards */
        buffer_invalidate_highlighting(buf, start_y);
    } else {
        /* Multi-line deletion */
        if (start_y >= (int)buf->num_rows || end_y >= (int)buf->num_rows) return;

        BufferRow *start_row = &buf->rows[start_y];
        BufferRow *end_row = &buf->rows[end_y];

        /* Clamp end_x to prevent underflow with unsigned arithmetic */
        if (end_x > (int)end_row->size) end_x = end_row->size;

        /* Keep start of first line and end of last line */
        size_t new_size = start_x + (end_row->size - end_x);
        while (start_row->capacity < new_size + 1) {
            start_row->capacity *= 2;
            char *new_data = realloc(start_row->data, start_row->capacity);
            if (!new_data) return;
            start_row->data = new_data;
        }

        /* Copy end part */
        memcpy(&start_row->data[start_x], &end_row->data[end_x],
               end_row->size - end_x);
        start_row->size = new_size;
        start_row->data[start_row->size] = '\0';

        /* Delete rows in between */
        for (int y = start_y + 1; y <= end_y; y++) {
            buffer_free_row(&buf->rows[y]);
        }

        /* Shift remaining rows */
        int rows_deleted = end_y - start_y;
        if (end_y + 1 < (int)buf->num_rows) {
            memmove(&buf->rows[start_y + 1], &buf->rows[end_y + 1],
                    sizeof(BufferRow) * (buf->num_rows - end_y - 1));
        }

        size_t old_rows = buf->num_rows;
        buf->num_rows -= rows_deleted;

        /* Resize highlighting cache after deleting rows */
        buffer_resize_highlighting_cache(buf, old_rows, buf->num_rows);

        buf->cursor_x = start_x;
        buf->cursor_y = start_y;
    }

    buf->has_selection = false;
    buf->modified = true;
}

void buffer_paste_text(Buffer *buf, const char *text, size_t len) {
    if (!buf || !text || len == 0) return;

    /* Find newlines in text */
    size_t line_start = 0;
    bool first_line = true;

    for (size_t i = 0; i <= len; i++) {
        if (i == len || text[i] == '\n') {
            size_t line_len = i - line_start;

            if (first_line) {
                /* Insert at cursor position */
                if (buf->cursor_y >= (int)buf->num_rows) {
                    buffer_append_row(buf, "", 0);
                }

                BufferRow *row = &buf->rows[buf->cursor_y];

                /* Ensure capacity */
                while (row->capacity < row->size + line_len + 1) {
                    row->capacity *= 2;
                    char *new_data = realloc(row->data, row->capacity);
                    if (!new_data) return;
                    row->data = new_data;
                }

                /* Insert text */
                memmove(&row->data[buf->cursor_x + line_len],
                        &row->data[buf->cursor_x],
                        row->size - buf->cursor_x);
                memcpy(&row->data[buf->cursor_x], &text[line_start], line_len);
                row->size += line_len;
                row->data[row->size] = '\0';

                buf->cursor_x += line_len;
                first_line = false;
            } else {
                /* New line - insert without auto-indent to preserve pasted formatting */
                if (buf->cursor_y >= (int)buf->num_rows) {
                    buffer_append_row(buf, "", 0);
                    buf->cursor_y++;
                } else {
                    /* Split current row at cursor without auto-indent */
                    BufferRow *row = &buf->rows[buf->cursor_y];
                    const char *rest = (buf->cursor_x < (int)row->size) ? &row->data[buf->cursor_x] : "";
                    size_t rest_len = (buf->cursor_x < (int)row->size) ? row->size - buf->cursor_x : 0;

                    /* Truncate current row */
                    row->size = buf->cursor_x;
                    row->data[row->size] = '\0';

                    /* Make space for new row */
                    if (buf->num_rows >= buf->capacity) {
                        size_t new_capacity = buf->capacity == 0 ? 16 : buf->capacity * 2;
                        BufferRow *new_rows = realloc(buf->rows, sizeof(BufferRow) * new_capacity);
                        if (!new_rows) return;
                        buf->rows = new_rows;
                        buf->capacity = new_capacity;
                    }

                    /* Shift rows down */
                    memmove(&buf->rows[buf->cursor_y + 2], &buf->rows[buf->cursor_y + 1],
                            sizeof(BufferRow) * (buf->num_rows - buf->cursor_y - 1));

                    /* Insert new row */
                    BufferRow *new_row = &buf->rows[buf->cursor_y + 1];
                    new_row->capacity = rest_len + 1;
                    new_row->data = malloc(new_row->capacity);
                    if (!new_row->data) return;

                    memcpy(new_row->data, rest, rest_len);
                    new_row->data[rest_len] = '\0';
                    new_row->size = rest_len;

                    buf->num_rows++;
                    buf->cursor_y++;
                }
                buf->cursor_x = 0;

                if (line_len > 0) {
                    BufferRow *row = &buf->rows[buf->cursor_y];

                    while (row->capacity < line_len + 1) {
                        row->capacity = row->capacity == 0 ? 128 : row->capacity * 2;
                        char *new_data = realloc(row->data, row->capacity);
                        if (!new_data) return;
                        row->data = new_data;
                    }

                    memcpy(row->data, &text[line_start], line_len);
                    row->size = line_len;
                    row->data[row->size] = '\0';
                    buf->cursor_x = line_len;
                }
            }

            line_start = i + 1;
        }
    }

    buf->modified = true;

    /* Invalidate all highlighting since we may have inserted multiple lines */
    if (buf->highlighted_lines) {
        buffer_invalidate_highlighting(buf, 0);
    }
}
