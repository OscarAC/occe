#ifndef GIT_H
#define GIT_H

#include <stdbool.h>
#include <stddef.h>

/* Git line status */
typedef enum {
    GIT_UNCHANGED = 0,
    GIT_ADDED,
    GIT_MODIFIED,
    GIT_DELETED
} GitLineStatus;

/* Git file status */
typedef struct {
    char *filename;
    char status;  /* M=modified, A=added, D=deleted, R=renamed, etc. */
} GitFileStatus;

/* Git repository info */
typedef struct GitRepo {
    char *root_path;       /* Repository root directory */
    char *current_branch;  /* Current branch name */
    bool is_repo;          /* Whether we're in a git repo */

    /* File status tracking */
    GitFileStatus *file_statuses;
    size_t file_count;
} GitRepo;

/* Git diff info for a file */
typedef struct GitDiff {
    GitLineStatus *line_statuses;  /* Per-line status */
    size_t num_lines;
} GitDiff;

/* Initialize git integration */
GitRepo *git_repo_open(const char *path);
void git_repo_close(GitRepo *repo);

/* Get current branch name */
char *git_get_branch(const char *repo_path);

/* Check if path is in a git repository */
bool git_is_repo(const char *path);

/* Get file status */
GitFileStatus *git_get_status(const char *repo_path, size_t *count);

/* Get diff for a specific file */
GitDiff *git_get_file_diff(const char *repo_path, const char *filename);
void git_diff_free(GitDiff *diff);

/* Helper to find git root from any path */
char *git_find_root(const char *path);

#endif /* GIT_H */
