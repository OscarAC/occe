#ifndef SEARCH_H
#define SEARCH_H

#include "buffer.h"

/* Search result */
typedef struct {
    int row;
    int col;
    size_t match_len;
} SearchResult;

/* Search for text in buffer */
SearchResult *buffer_search(Buffer *buf, const char *query, int start_row, int start_col, bool forward);

/* Search and replace */
int buffer_replace(Buffer *buf, const char *search, const char *replace, bool all);

#endif /* SEARCH_H */
