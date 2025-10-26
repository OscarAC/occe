#define _POSIX_C_SOURCE 200809L
#include "occe.h"
#include "buffer.h"
#include "window.h"
#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    Editor *ed = editor_create();
    if (!ed) {
        fprintf(stderr, "Failed to initialize editor\n");
        return 1;
    }

    /* If a filename was provided, open it */
    if (argc >= 2) {
        Buffer *buf = buffer_create();
        if (buf) {
            if (buffer_open(buf, argv[1]) == -1) {
                /* File doesn't exist, create new buffer with filename */
                buf->filename = strdup(argv[1]);
                buffer_append_row(buf, "", 0);
            }

            /* Add buffer to editor */
            ed->buffers = malloc(sizeof(Buffer *));
            ed->buffers[0] = buf;
            ed->buffer_count = 1;

            /* Create window for buffer (leave room for command line) */
            ed->root_window = window_create_leaf(buf, 0, 0, ed->term->cols, ed->term->rows - 1);
            ed->root_window->id = ed->next_window_id++;
            ed->active_window = ed->root_window;

            /* Create initial tab group */
            ed->active_tab = tabgroup_create("Main", ed->root_window);
            ed->active_tab->id = ed->next_tab_id++;
            ed->tab_groups = ed->active_tab;
        }
    }

    /* Run editor */
    int result = editor_run(ed);

    /* Cleanup */
    editor_destroy(ed);

    return result;
}
