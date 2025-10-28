#define _POSIX_C_SOURCE 200809L
#include "search.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>

SearchResult *buffer_search(Buffer *buf, const char *query, int start_row, int start_col, bool forward) {
    if (!buf || !query || !query[0]) return NULL;

    size_t query_len = strlen(query);
    int row = start_row;
    int col = start_col;

    if (forward) {
        /* Search forward - skip current position to find next match */
        /* Start searching from col+1 on first row to be consistent with backward search */
        int search_start = (row == start_row && col < (int)buf->rows[row].size) ? col + 1 : col;

        for (; row < (int)buf->num_rows; row++) {
            if (row >= (int)buf->num_rows) break;

            BufferRow *r = &buf->rows[row];
            const char *found = strstr(r->data + search_start, query);

            if (found) {
                SearchResult *result = malloc(sizeof(SearchResult));
                if (!result) return NULL;

                result->row = row;
                result->col = found - r->data;
                result->match_len = query_len;
                return result;
            }

            search_start = 0; /* Start from beginning of next lines */
        }
    } else {
        /* Search backward */
        for (; row >= 0; row--) {
            if (row < 0 || row >= (int)buf->num_rows) break;

            BufferRow *r = &buf->rows[row];

            /* Search backwards in the current line */
            int search_col = (row == start_row) ? col : (int)r->size;

            for (int c = search_col - 1; c >= 0; c--) {
                if (c + query_len <= r->size) {
                    if (strncmp(r->data + c, query, query_len) == 0) {
                        SearchResult *result = malloc(sizeof(SearchResult));
                        if (!result) return NULL;

                        result->row = row;
                        result->col = c;
                        result->match_len = query_len;
                        return result;
                    }
                }
            }
        }
    }

    return NULL; /* Not found */
}

int buffer_replace(Buffer *buf, const char *search, const char *replace, bool all) {
    if (!buf || !search || !replace) return 0;

    int count = 0;
    int row = 0;
    int col = 0;

    size_t search_len = strlen(search);
    size_t replace_len = strlen(replace);

    do {
        SearchResult *result = buffer_search(buf, search, row, col, true);
        if (!result) break;

        /* Found a match at result->row, result->col */
        BufferRow *r = &buf->rows[result->row];

        /* Calculate new size */
        size_t new_size = r->size - search_len + replace_len;

        /* Ensure capacity */
        while (new_size >= r->capacity) {
            r->capacity *= 2;
            char *new_data = realloc(r->data, r->capacity);
            if (!new_data) {
                free(result);
                return count;
            }
            r->data = new_data;
        }

        /* Make room for replacement */
        memmove(r->data + result->col + replace_len,
                r->data + result->col + search_len,
                r->size - result->col - search_len);

        /* Insert replacement */
        memcpy(r->data + result->col, replace, replace_len);
        r->size = new_size;
        r->data[r->size] = '\0';

        count++;
        row = result->row;
        col = result->col + replace_len;

        free(result);
    } while (all);

    if (count > 0) {
        buf->modified = true;
    }

    return count;
}
