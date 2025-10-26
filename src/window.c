#define _POSIX_C_SOURCE 200809L
#include "window.h"
#include "terminal.h"
#include "syntax.h"
#include "colors.h"
#include "lua_bridge.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Window *window_create_leaf(Buffer *buf, int x, int y, int w, int h) {
    Window *win = malloc(sizeof(Window));
    if (!win) return NULL;

    win->type = WINDOW_LEAF;
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->id = 0;  /* Will be set by caller */

    /* Content */
    win->content_type = CONTENT_BUFFER;
    win->content.buffer = buf;
    win->renderer_name = NULL;
    win->row_offset = 0;
    win->col_offset = 0;

    /* Split */
    win->left = NULL;
    win->right = NULL;
    win->split_ratio = 0.5f;

    /* Layout hints */
    win->min_width = 10;
    win->min_height = 3;
    win->max_width = -1;  /* Unlimited */
    win->max_height = -1;
    win->weight = 1.0f;
    win->focused = false;

    return win;
}

Window *window_create_leaf_custom(const char *renderer, int x, int y, int w, int h) {
    Window *win = malloc(sizeof(Window));
    if (!win) return NULL;

    win->type = WINDOW_LEAF;
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->id = 0;  /* Will be set by caller */

    /* Content */
    win->content_type = CONTENT_CUSTOM;
    win->content.custom_data = NULL;  /* Will be set from Lua */
    win->renderer_name = renderer ? strdup(renderer) : NULL;
    win->row_offset = 0;
    win->col_offset = 0;

    /* Split */
    win->left = NULL;
    win->right = NULL;
    win->split_ratio = 0.5f;

    /* Layout hints */
    win->min_width = 10;
    win->min_height = 3;
    win->max_width = -1;
    win->max_height = -1;
    win->weight = 1.0f;
    win->focused = false;

    return win;
}

Window *window_create_split(WindowType type, Window *left, Window *right, float ratio) {
    Window *win = malloc(sizeof(Window));
    if (!win) return NULL;

    win->type = type;
    win->id = 0;  /* Will be set by caller */

    /* Content - splits don't have content */
    win->content_type = CONTENT_BUFFER;
    win->content.buffer = NULL;
    win->renderer_name = NULL;
    win->row_offset = 0;
    win->col_offset = 0;

    /* Split */
    win->left = left;
    win->right = right;
    win->split_ratio = ratio;

    /* Layout hints */
    win->min_width = 10;
    win->min_height = 3;
    win->max_width = -1;
    win->max_height = -1;
    win->weight = 1.0f;
    win->focused = false;

    return win;
}

void window_destroy(Window *win) {
    if (!win) return;

    if (win->type != WINDOW_LEAF) {
        window_destroy(win->left);
        window_destroy(win->right);
    }

    /* Free renderer name if it exists */
    if (win->renderer_name) {
        free(win->renderer_name);
    }

    free(win);
}

void window_resize(Window *win, int x, int y, int w, int h) {
    if (!win) return;

    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;

    if (win->type == WINDOW_SPLIT_H) {
        int split_pos = (int)(h * win->split_ratio);
        if (win->left) window_resize(win->left, x, y, w, split_pos);
        if (win->right) window_resize(win->right, x, y + split_pos, w, h - split_pos);
    } else if (win->type == WINDOW_SPLIT_V) {
        int split_pos = (int)(w * win->split_ratio);
        if (win->left) window_resize(win->left, x, y, split_pos, h);
        if (win->right) window_resize(win->right, x + split_pos, y, w - split_pos, h);
    }
}

static void window_render_leaf(Window *win, Terminal *term, Editor *ed, bool show_line_numbers) {
    /* Handle custom renderers */
    if (win->content_type == CONTENT_CUSTOM && win->renderer_name) {
        /* Call Lua renderer */
        lua_bridge_call_window_renderer(ed, win);
        return;
    }

    if (!win->content.buffer) return;

    Buffer *buf = win->content.buffer;

    /* Calculate line number gutter width */
    int gutter_width = 0;
    if (show_line_numbers && buf->num_rows > 0) {
        int max_line = buf->num_rows;
        gutter_width = snprintf(NULL, 0, "%d", max_line) + 1; /* +1 for space */
        /* Add 2 more for git symbol + space */
        gutter_width += 2;
    }

    /* Adjust scroll offset to keep cursor in view */
    if (buf->cursor_y < win->row_offset) {
        win->row_offset = buf->cursor_y;
    }
    if (buf->cursor_y >= win->row_offset + win->height - 1) {
        win->row_offset = buf->cursor_y - win->height + 2;
    }

    /* Find matching bracket for highlighting */
    BracketMatch bracket_match = buffer_find_matching_bracket(buf);

    /* Render visible rows */
    for (int y = 0; y < win->height - 1; y++) {
        int file_row = y + win->row_offset;

        terminal_move_cursor(term, win->y + y, win->x);

        /* Render line number gutter */
        if (show_line_numbers) {
            char line_num[16];
            if (file_row >= (int)buf->num_rows) {
                /* Empty line - just spaces */
                for (int i = 0; i < gutter_width; i++) {
                    terminal_write_str(term, " ");
                }
            } else {
                /* Draw line number */
                bool is_cursor_line = (file_row == buf->cursor_y);

                /* Set color: bright for cursor line, dim for others */
                if (is_cursor_line) {
                    terminal_write_str(term, "\x1b[1;33m"); /* Bold yellow */
                } else {
                    terminal_write_str(term, "\x1b[90m"); /* Bright black (gray) */
                }

                /* Format and write line number (excluding git space) */
                int line_num_width = gutter_width - 3;  /* -2 for git, -1 for original space */
                int len = snprintf(line_num, sizeof(line_num), "%*d ",
                                   line_num_width, file_row + 1);
                terminal_write(term, line_num, len);

                /* Reset color */
                terminal_write_str(term, "\x1b[0m");

                /* Render gutter via Lua plugin (e.g., git status) */
                char *gutter = lua_bridge_call_gutter_renderer(ed, file_row);
                if (gutter) {
                    terminal_write_str(term, gutter);
                    free(gutter);
                } else {
                    terminal_write_str(term, "  ");  /* No gutter renderer */
                }
            }
        }

        if (file_row >= (int)buf->num_rows) {
            /* Empty row */
            terminal_write_str(term, "~");
            /* Clear rest of line */
            terminal_write_str(term, "\x1b[K");
        } else {
            /* Render text row with syntax highlighting */
            BufferRow *row = &buf->rows[file_row];

            /* Set current line background */
            bool is_cursor_line = (file_row == buf->cursor_y);
            if (is_cursor_line) {
                terminal_write_str(term, "\x1b[100m"); /* Bright black (gray) background */
            }

            /* Get or compute highlighting for this line */
            HighlightedLine *hl = NULL;
            if (buf->syntax) {
                /* Check cache */
                if (buf->highlighted_lines && buf->highlighted_lines[file_row]) {
                    hl = buf->highlighted_lines[file_row];
                } else {
                    /* Compute highlighting */
                    bool prev_multiline = (file_row > 0 && buf->multiline_states) ?
                                         buf->multiline_states[file_row - 1] : false;
                    hl = syntax_highlight_line(buf->syntax, row->data, prev_multiline);

                    /* Cache it */
                    if (buf->highlighted_lines) {
                        buf->highlighted_lines[file_row] = hl;
                        if (buf->multiline_states) {
                            buf->multiline_states[file_row] = hl->in_multiline;
                        }
                    }
                }
            }

            /* Render the line with colors */
            int col = 0;
            int visible_start = win->col_offset;
            int visible_end = visible_start + win->width - gutter_width;

            if (hl && hl->num_segments > 0) {
                /* Render with syntax highlighting */
                for (size_t seg_idx = 0; seg_idx < hl->num_segments; seg_idx++) {
                    HighlightSegment *seg = &hl->segments[seg_idx];

                    /* Render any text before this segment as normal */
                    if (col < seg->start) {
                        int start = col < visible_start ? visible_start : col;
                        int end = seg->start < visible_end ? seg->start : visible_end;
                        if (start < end && start < (int)row->size) {
                            terminal_write_str(term, "\x1b[0m"); /* Reset */
                            int write_len = end - start;
                            if (write_len > 0) {
                                terminal_write(term, row->data + start, write_len);
                            }
                        }
                        col = seg->start;
                    }

                    /* Render the highlighted segment */
                    if (col < visible_end && seg->end > visible_start) {
                        int start = col < visible_start ? visible_start : col;
                        int end = seg->end < visible_end ? seg->end : visible_end;
                        if (start < end && start < (int)row->size) {
                            /* Set color */
                            ColorPair cp = colors_get(seg->type);
                            char color_code[32];
                            colors_to_ansi(cp, color_code, sizeof(color_code));
                            terminal_write_str(term, color_code);

                            /* Write text */
                            int write_len = end - start;
                            terminal_write(term, row->data + start, write_len);

                            /* Reset color */
                            terminal_write_str(term, "\x1b[0m");
                        }
                        col = seg->end;
                    }
                }

                /* Render any remaining text as normal */
                if (col < visible_end && col < (int)row->size) {
                    int start = col < visible_start ? visible_start : col;
                    int end = visible_end < (int)row->size ? visible_end : (int)row->size;
                    if (start < end) {
                        terminal_write(term, row->data + start, end - start);
                    }
                }
            } else {
                /* No highlighting - render normally */
                int len = row->size - win->col_offset;
                if (len < 0) len = 0;
                if (len > win->width) len = win->width;

                if (len > 0) {
                    terminal_write(term, row->data + win->col_offset, len);
                }
            }

            /* Highlight visual selection */
            if (buf->has_selection) {
                int start_y = buf->select_start_y;
                int start_x = buf->select_start_x;
                int end_y = buf->cursor_y;
                int end_x = buf->cursor_x;

                /* Normalize selection */
                if (start_y > end_y || (start_y == end_y && start_x > end_x)) {
                    int tmp = start_y; start_y = end_y; end_y = tmp;
                    tmp = start_x; start_x = end_x; end_x = tmp;
                }

                /* Check if this line is in selection */
                if (file_row >= start_y && file_row <= end_y) {
                    int sel_start_col, sel_end_col;

                    if (file_row == start_y && file_row == end_y) {
                        sel_start_col = start_x;
                        sel_end_col = end_x;
                    } else if (file_row == start_y) {
                        sel_start_col = start_x;
                        sel_end_col = row->size;
                    } else if (file_row == end_y) {
                        sel_start_col = 0;
                        sel_end_col = end_x;
                    } else {
                        sel_start_col = 0;
                        sel_end_col = row->size;
                    }

                    /* Highlight the selection on this line */
                    for (int col = sel_start_col; col < sel_end_col && col < (int)row->size; col++) {
                        if (col >= win->col_offset && col < win->col_offset + win->width - gutter_width) {
                            int screen_col = win->x + gutter_width + (col - win->col_offset);
                            terminal_move_cursor(term, win->y + y, screen_col);
                            terminal_write_str(term, "\x1b[7m");  /* Reverse video */
                            terminal_write(term, &row->data[col], 1);
                            terminal_write_str(term, "\x1b[27m"); /* Reset reverse */
                        }
                    }
                }
            }

            /* Highlight matching brackets */
            if (bracket_match.found) {
                /* Highlight bracket at cursor */
                if (file_row == buf->cursor_y && buf->cursor_x >= win->col_offset &&
                    buf->cursor_x < win->col_offset + win->width - gutter_width) {
                    int screen_col = win->x + gutter_width + (buf->cursor_x - win->col_offset);
                    terminal_move_cursor(term, win->y + y, screen_col);
                    terminal_write_str(term, "\x1b[7m");  /* Reverse video */
                    terminal_write(term, &row->data[buf->cursor_x], 1);
                    terminal_write_str(term, "\x1b[27m"); /* Reset reverse */
                }

                /* Highlight matching bracket */
                if (file_row == bracket_match.row && bracket_match.col >= win->col_offset &&
                    bracket_match.col < win->col_offset + win->width - gutter_width) {
                    int screen_col = win->x + gutter_width + (bracket_match.col - win->col_offset);
                    terminal_move_cursor(term, win->y + y, screen_col);
                    terminal_write_str(term, "\x1b[7m");  /* Reverse video */
                    BufferRow *match_row = &buf->rows[bracket_match.row];
                    terminal_write(term, &match_row->data[bracket_match.col], 1);
                    terminal_write_str(term, "\x1b[27m"); /* Reset reverse */
                }
            }

            /* Move to end of line for clearing */
            terminal_move_cursor(term, win->y + y, win->x + win->width - 1);

            /* Clear rest of line */
            terminal_write_str(term, "\x1b[K");

            /* Reset background if this was the cursor line */
            if (is_cursor_line) {
                terminal_write_str(term, "\x1b[0m");
            }
        }
    }

    /* Render status line */
    terminal_move_cursor(term, win->y + win->height - 1, win->x);
    terminal_write_str(term, "\x1b[7m"); /* Invert colors */

    char status[256];
    int len = snprintf(status, sizeof(status), " %s %s| %d:%d ",
                   buf->filename ? buf->filename : "[No Name]",
                   buf->modified ? "[+] " : "",
                   buf->cursor_y + 1, buf->cursor_x + 1);

    if (len > win->width) len = win->width;
    terminal_write(term, status, len);

    /* Fill rest of status line */
    for (int i = len; i < win->width; i++) {
        terminal_write_str(term, " ");
    }

    terminal_write_str(term, "\x1b[m"); /* Reset colors */
}

void window_render(Window *win, Terminal *term, Editor *ed, bool show_line_numbers) {
    if (!win) return;

    if (win->type == WINDOW_LEAF) {
        window_render_leaf(win, term, ed, show_line_numbers);
    } else {
        window_render(win->left, term, ed, show_line_numbers);
        window_render(win->right, term, ed, show_line_numbers);
    }
}

/* Helper function to collect all leaf windows in-order */
static void window_collect_leaves(Window *win, Window **leaves, size_t *count, size_t max) {
    if (!win || *count >= max) return;

    if (win->type == WINDOW_LEAF) {
        leaves[(*count)++] = win;
    } else {
        window_collect_leaves(win->left, leaves, count, max);
        window_collect_leaves(win->right, leaves, count, max);
    }
}

Window *window_find_leaf(Window *win, Window *target) {
    if (!win) return NULL;
    if (win == target && win->type == WINDOW_LEAF) return win;

    if (win->type != WINDOW_LEAF) {
        Window *found = window_find_leaf(win->left, target);
        if (found) return found;
        return window_find_leaf(win->right, target);
    }

    return NULL;
}

Window *window_get_next_leaf(Window *root, Window *current) {
    if (!root || !current) return NULL;

    /* Collect all leaves - use dynamic allocation */
    size_t max_leaves = 256;
    Window **leaves = malloc(sizeof(Window *) * max_leaves);
    if (!leaves) return NULL;

    size_t count = 0;
    window_collect_leaves(root, leaves, &count, max_leaves);

    /* Find current window in list */
    Window *result = NULL;
    for (size_t i = 0; i < count; i++) {
        if (leaves[i] == current) {
            /* Return next, wrapping around */
            result = leaves[(i + 1) % count];
            break;
        }
    }

    free(leaves);
    return result;
}

Window *window_get_prev_leaf(Window *root, Window *current) {
    if (!root || !current) return NULL;

    /* Collect all leaves - use dynamic allocation */
    size_t max_leaves = 256;
    Window **leaves = malloc(sizeof(Window *) * max_leaves);
    if (!leaves) return NULL;

    size_t count = 0;
    window_collect_leaves(root, leaves, &count, max_leaves);

    /* Find current window in list */
    Window *result = NULL;
    for (size_t i = 0; i < count; i++) {
        if (leaves[i] == current) {
            /* Return previous, wrapping around */
            result = leaves[(i + count - 1) % count];
            break;
        }
    }

    free(leaves);
    return result;
}

Window *window_close_split(Window *root, Window *to_close, Window **new_active) {
    /* Simple implementation: Can't close if it's the only window */
    if (!root || !to_close || root == to_close) {
        if (new_active) *new_active = root;
        return root;
    }

    /* Find the parent of to_close */
    /* For now, just return root unchanged if complex */
    /* This is a simplified version */
    if (new_active) *new_active = root;
    return root;
}

Window *window_only(Window *root, Window *to_keep) {
    if (!root || !to_keep || to_keep->type != WINDOW_LEAF) return root;

    /* Save window properties */
    int x = root->x;
    int y = root->y;
    int w = root->width;
    int h = root->height;
    int id = to_keep->id;

    /* Create a new single window based on content type */
    Window *new_win;
    if (to_keep->content_type == CONTENT_BUFFER) {
        Buffer *buf = to_keep->content.buffer;
        new_win = window_create_leaf(buf, x, y, w, h);
    } else {
        new_win = window_create_leaf_custom(to_keep->renderer_name, x, y, w, h);
        new_win->content.custom_data = to_keep->content.custom_data;
    }
    new_win->id = id;

    /* Destroy old tree */
    window_destroy(root);

    return new_win;
}

/* Find window by ID */
Window *window_find_by_id(Window *root, int id) {
    if (!root) return NULL;
    if (root->id == id) return root;

    if (root->type != WINDOW_LEAF) {
        Window *found = window_find_by_id(root->left, id);
        if (found) return found;
        return window_find_by_id(root->right, id);
    }

    return NULL;
}

/* Set focused state for a window */
void window_set_focused(Window *win, bool focused) {
    if (!win) return;
    win->focused = focused;
}

/* Get window in a direction (left/right/up/down) */
Window *window_get_direction(Window *root, Window *current, const char *direction) {
    /* Simplified implementation - just cycle for now */
    /* TODO: Implement proper directional navigation */
    if (!root || !current || !direction) return NULL;

    if (strcmp(direction, "left") == 0 || strcmp(direction, "up") == 0) {
        return window_get_prev_leaf(root, current);
    } else if (strcmp(direction, "right") == 0 || strcmp(direction, "down") == 0) {
        return window_get_next_leaf(root, current);
    }

    return NULL;
}

/* Swap two windows' content */
void window_swap(Window *a, Window *b) {
    if (!a || !b || a->type != WINDOW_LEAF || b->type != WINDOW_LEAF) return;

    /* Swap content type */
    ContentType temp_type = a->content_type;
    a->content_type = b->content_type;
    b->content_type = temp_type;

    /* Swap content union */
    if (a->content_type == CONTENT_BUFFER && b->content_type == CONTENT_BUFFER) {
        Buffer *temp = a->content.buffer;
        a->content.buffer = b->content.buffer;
        b->content.buffer = temp;
    } else {
        void *temp_data = a->content.custom_data;
        a->content.custom_data = b->content.custom_data;
        b->content.custom_data = temp_data;

        char *temp_name = a->renderer_name;
        a->renderer_name = b->renderer_name;
        b->renderer_name = temp_name;
    }
}

/* Set split ratio for a split window */
void window_set_split_ratio(Window *win, float ratio) {
    if (!win || win->type == WINDOW_LEAF) return;
    if (ratio < 0.1f) ratio = 0.1f;
    if (ratio > 0.9f) ratio = 0.9f;
    win->split_ratio = ratio;
}

/* Tab group operations */
TabGroup *tabgroup_create(const char *name, Window *root) {
    TabGroup *group = malloc(sizeof(TabGroup));
    if (!group) return NULL;

    group->name = name ? strdup(name) : strdup("Tab");
    group->id = 0;  /* Will be set by caller */
    group->root_window = root;
    group->active_window = root;
    group->next = NULL;

    return group;
}

void tabgroup_destroy(TabGroup *group) {
    if (!group) return;

    if (group->name) free(group->name);
    if (group->root_window) window_destroy(group->root_window);
    free(group);
}

TabGroup *tabgroup_find_by_id(TabGroup *head, int id) {
    TabGroup *current = head;
    while (current) {
        if (current->id == id) return current;
        current = current->next;
    }
    return NULL;
}

/* Advanced layout operations */

/* Count number of leaf windows */
int window_count_leaves(Window *win) {
    if (!win) return 0;
    if (win->type == WINDOW_LEAF) return 1;
    return window_count_leaves(win->left) + window_count_leaves(win->right);
}

/* Collect all leaf windows into an array */
void window_collect_all_leaves(Window *win, Window **leaves, int *count, int max) {
    if (!win || !leaves || !count || *count >= max) return;

    if (win->type == WINDOW_LEAF) {
        leaves[(*count)++] = win;
    } else {
        window_collect_all_leaves(win->left, leaves, count, max);
        window_collect_all_leaves(win->right, leaves, count, max);
    }
}

/* Equalize all split ratios to distribute space evenly */
void window_equalize_sizes(Window *win) {
    if (!win || win->type == WINDOW_LEAF) return;

    /* Count leaves in left and right subtrees */
    int left_count = window_count_leaves(win->left);
    int right_count = window_count_leaves(win->right);
    int total = left_count + right_count;

    if (total > 0) {
        /* Set ratio proportional to number of leaves */
        win->split_ratio = (float)left_count / (float)total;
    } else {
        win->split_ratio = 0.5f;
    }

    /* Recursively equalize children */
    window_equalize_sizes(win->left);
    window_equalize_sizes(win->right);
}

/* Find parent of a window */
Window *window_find_parent(Window *root, Window *child) {
    if (!root || !child || root == child) return NULL;

    if (root->type != WINDOW_LEAF) {
        if (root->left == child || root->right == child) {
            return root;
        }

        Window *found = window_find_parent(root->left, child);
        if (found) return found;

        return window_find_parent(root->right, child);
    }

    return NULL;
}

/* Find sibling of a window */
Window *window_find_sibling(Window *root, Window *target) {
    if (!root || !target) return NULL;

    Window *parent = window_find_parent(root, target);
    if (!parent) return NULL;

    if (parent->left == target) {
        return parent->right;
    } else {
        return parent->left;
    }
}
