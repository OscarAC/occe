#define _POSIX_C_SOURCE 200809L
#include "git.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>

/* Helper to run git command and get output */
static char *run_git_command(const char *repo_path, const char *cmd) {
    /* Validate repo_path doesn't contain shell metacharacters that could escape quotes */
    if (repo_path && strchr(repo_path, '\'')) {
        return NULL;  /* Reject paths with single quotes to prevent command injection */
    }

    char full_cmd[1024];
    snprintf(full_cmd, sizeof(full_cmd), "cd '%s' && git %s 2>/dev/null", repo_path, cmd);

    FILE *fp = popen(full_cmd, "r");
    if (!fp) return NULL;

    char *result = NULL;
    size_t size = 0;
    size_t capacity = 1024;
    result = malloc(capacity);
    if (!result) {
        pclose(fp);
        return NULL;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp)) {
        size_t len = strlen(buffer);
        if (size + len >= capacity) {
            capacity *= 2;
            char *new_result = realloc(result, capacity);
            if (!new_result) {
                free(result);
                pclose(fp);
                return NULL;
            }
            result = new_result;
        }
        memcpy(result + size, buffer, len);
        size += len;
    }

    pclose(fp);
    result[size] = '\0';
    return result;
}

char *git_find_root(const char *path) {
    if (!path) return NULL;

    char *test_path = strdup(path);
    if (!test_path) return NULL;

    /* If path is a file, get its directory */
    struct stat st;
    if (stat(test_path, &st) == 0 && !S_ISDIR(st.st_mode)) {
        /* dirname may modify buffer or return static storage - make a copy first */
        char *path_copy = strdup(test_path);
        if (!path_copy) {
            free(test_path);
            return NULL;
        }
        char *dir = dirname(path_copy);
        char *dir_copy = strdup(dir);
        free(path_copy);
        free(test_path);
        test_path = dir_copy;
        if (!test_path) return NULL;
    }

    /* Walk up directory tree looking for .git */
    while (test_path && strlen(test_path) > 1) {
        char git_dir[1024];
        snprintf(git_dir, sizeof(git_dir), "%s/.git", test_path);

        if (stat(git_dir, &st) == 0) {
            return test_path;  /* Found .git directory */
        }

        /* Go up one level */
        char *parent = dirname(test_path);
        if (strcmp(parent, test_path) == 0) {
            break;  /* Reached root */
        }

        char *new_path = strdup(parent);
        free(test_path);
        test_path = new_path;
    }

    free(test_path);
    return NULL;
}

bool git_is_repo(const char *path) {
    char *root = git_find_root(path);
    if (root) {
        free(root);
        return true;
    }
    return false;
}

char *git_get_branch(const char *repo_path) {
    char *output = run_git_command(repo_path, "branch --show-current");
    if (!output) return NULL;

    /* Remove trailing newline */
    size_t len = strlen(output);
    if (len > 0 && output[len - 1] == '\n') {
        output[len - 1] = '\0';
    }

    return output;
}

GitRepo *git_repo_open(const char *path) {
    if (!path) return NULL;

    char *root = git_find_root(path);
    if (!root) return NULL;

    GitRepo *repo = malloc(sizeof(GitRepo));
    if (!repo) {
        free(root);
        return NULL;
    }

    repo->root_path = root;
    repo->current_branch = git_get_branch(root);
    repo->is_repo = true;
    repo->file_statuses = NULL;
    repo->file_count = 0;

    return repo;
}

void git_repo_close(GitRepo *repo) {
    if (!repo) return;

    free(repo->root_path);
    free(repo->current_branch);

    for (size_t i = 0; i < repo->file_count; i++) {
        free(repo->file_statuses[i].filename);
    }
    free(repo->file_statuses);

    free(repo);
}

GitFileStatus *git_get_status(const char *repo_path, size_t *count) {
    if (!repo_path || !count) return NULL;

    char *output = run_git_command(repo_path, "status --porcelain");
    if (!output) {
        *count = 0;
        return NULL;
    }

    /* Count lines */
    size_t num_files = 0;
    for (char *p = output; *p; p++) {
        if (*p == '\n') num_files++;
    }

    if (num_files == 0) {
        free(output);
        *count = 0;
        return NULL;
    }

    GitFileStatus *statuses = malloc(sizeof(GitFileStatus) * num_files);
    if (!statuses) {
        free(output);
        *count = 0;
        return NULL;
    }

    /* Parse lines */
    size_t idx = 0;
    char *line = strtok(output, "\n");
    while (line && idx < num_files) {
        if (strlen(line) > 3) {
            statuses[idx].status = line[1] != ' ' ? line[1] : line[0];
            statuses[idx].filename = strdup(line + 3);
            idx++;
        }
        line = strtok(NULL, "\n");
    }

    free(output);
    *count = idx;
    return statuses;
}

GitDiff *git_get_file_diff(const char *repo_path, const char *filename) {
    if (!repo_path || !filename) return NULL;

    /* Get the file content line count first */
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", repo_path, filename);

    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    size_t num_lines = 0;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), f)) {
        num_lines++;
    }
    fclose(f);

    GitDiff *diff = malloc(sizeof(GitDiff));
    if (!diff) return NULL;

    diff->num_lines = num_lines;
    diff->line_statuses = calloc(num_lines, sizeof(GitLineStatus));
    if (!diff->line_statuses) {
        free(diff);
        return NULL;
    }

    /* Get git diff */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "diff HEAD -- '%s'", filename);
    char *output = run_git_command(repo_path, cmd);

    if (!output) {
        /* No diff means file is unchanged or new */
        /* Check if file is tracked */
        snprintf(cmd, sizeof(cmd), "ls-files '%s'", filename);
        char *tracked = run_git_command(repo_path, cmd);
        if (tracked && strlen(tracked) > 0) {
            /* File is tracked but unchanged */
            free(tracked);
        } else {
            /* File is new (untracked or added) */
            for (size_t i = 0; i < num_lines; i++) {
                diff->line_statuses[i] = GIT_ADDED;
            }
        }
        free(tracked);
        return diff;
    }

    /* Parse diff output
     * Format: @@ -old_start,old_count +new_start,new_count @@
     */
    char *line = strtok(output, "\n");
    int current_line = 0;
    bool in_hunk = false;

    while (line) {
        if (strncmp(line, "@@", 2) == 0) {
            /* Parse hunk header */
            int new_start, new_count;
            if (sscanf(line, "@@ -%*d,%*d +%d,%d @@", &new_start, &new_count) == 2) {
                current_line = new_start - 1;  /* Convert to 0-based */
                in_hunk = true;
            }
        } else if (in_hunk && strlen(line) > 0) {
            char marker = line[0];
            if (marker == '+') {
                /* Added line - exists in new file */
                if (current_line < (int)num_lines) {
                    diff->line_statuses[current_line] = GIT_ADDED;
                    current_line++;
                }
            } else if (marker == '-') {
                /* Deleted line - doesn't exist in new file, don't mark or advance */
                /* These lines are in the old version but not in the new version */
            } else if (marker == ' ') {
                /* Unchanged line - advance position in new file */
                current_line++;
            }
        }
        line = strtok(NULL, "\n");
    }

    free(output);
    return diff;
}

void git_diff_free(GitDiff *diff) {
    if (!diff) return;
    free(diff->line_statuses);
    free(diff);
}
