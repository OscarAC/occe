#define _POSIX_C_SOURCE 200809L
#include "occe.h"
#include "buffer.h"
#include "window.h"
#include "terminal.h"
#include "git.h"
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

            /* Initialize git integration */
            ed->git_repo = git_repo_open(argv[1]);
            if (ed->git_repo && buf->filename) {
                /* Set branch name */
                if (ed->git_repo->current_branch) {
                    buf->git_branch = strdup(ed->git_repo->current_branch);
                }

                /* Load git diff for this file */
                char *filename = buf->filename;
                /* Make filename relative to repo root */
                if (ed->git_repo->root_path) {
                    size_t root_len = strlen(ed->git_repo->root_path);
                    if (strncmp(filename, ed->git_repo->root_path, root_len) == 0) {
                        filename += root_len;
                        if (*filename == '/') filename++;
                    }
                }
                buf->git_diff = git_get_file_diff(ed->git_repo->root_path, filename);
            }

            /* Create window for buffer (leave room for command line) */
            ed->root_window = window_create_leaf(buf, 0, 0, ed->term->cols, ed->term->rows - 1);
            ed->active_window = ed->root_window;
        }
    }

    /* Run editor */
    int result = editor_run(ed);

    /* Cleanup */
    editor_destroy(ed);

    return result;
}
