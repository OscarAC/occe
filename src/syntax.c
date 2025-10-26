#define _POSIX_C_SOURCE 200809L
#include "syntax.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

Syntax *syntax_list = NULL;

void syntax_init(void) {
    /* Initialize built-in syntaxes */
}

Syntax *syntax_register(const char *name) {
    Syntax *syn = malloc(sizeof(Syntax));
    if (!syn) return NULL;

    syn->name = strdup(name);
    syn->extensions = NULL;
    syn->num_extensions = 0;
    syn->rules = NULL;
    syn->num_rules = 0;
    syn->rules_capacity = 0;
    syn->singleline_comment = NULL;
    syn->multiline_start = NULL;
    syn->multiline_end = NULL;
    syn->next = syntax_list;
    syntax_list = syn;

    return syn;
}

void syntax_add_extension(Syntax *syn, const char *ext) {
    if (!syn) return;

    char **new_extensions = realloc(syn->extensions, sizeof(char *) * (syn->num_extensions + 1));
    if (!new_extensions) return;  /* Allocation failed, keep old data */

    syn->extensions = new_extensions;
    syn->extensions[syn->num_extensions] = strdup(ext);
    syn->num_extensions++;
}

void syntax_add_rule(Syntax *syn, PatternType type, const char *pattern, HighlightType hl_type) {
    if (!syn) return;

    if (syn->num_rules >= syn->rules_capacity) {
        size_t new_capacity = syn->rules_capacity == 0 ? 32 : syn->rules_capacity * 2;
        SyntaxRule *new_rules = realloc(syn->rules, sizeof(SyntaxRule) * new_capacity);
        if (!new_rules) return;  /* Allocation failed, keep old data */
        syn->rules = new_rules;
        syn->rules_capacity = new_capacity;
    }

    SyntaxRule *rule = &syn->rules[syn->num_rules];
    rule->type = type;
    rule->pattern = strdup(pattern);
    rule->hl_type = hl_type;
    rule->priority = 0;
    syn->num_rules++;
}

void syntax_add_keyword(Syntax *syn, const char *keyword, HighlightType hl_type) {
    syntax_add_rule(syn, PATTERN_KEYWORD, keyword, hl_type);
}

void syntax_set_comments(Syntax *syn, const char *single, const char *multi_start, const char *multi_end) {
    if (!syn) return;

    if (single) syn->singleline_comment = strdup(single);
    if (multi_start) syn->multiline_start = strdup(multi_start);
    if (multi_end) syn->multiline_end = strdup(multi_end);
}

Syntax *syntax_find_by_filename(const char *filename) {
    if (!filename) return NULL;

    /* Find file extension */
    const char *dot = strrchr(filename, '.');
    if (!dot) return NULL;

    /* Search for matching syntax */
    for (Syntax *syn = syntax_list; syn; syn = syn->next) {
        for (size_t i = 0; i < syn->num_extensions; i++) {
            if (strcmp(dot, syn->extensions[i]) == 0) {
                return syn;
            }
        }
    }

    return NULL;
}

static bool is_separator(int c) {
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];{}:", c) != NULL;
}

static void add_segment(HighlightedLine *hl, int start, int end, HighlightType type) {
    if (hl->num_segments >= hl->capacity) {
        size_t new_capacity = hl->capacity == 0 ? 16 : hl->capacity * 2;
        HighlightSegment *new_segments = realloc(hl->segments, sizeof(HighlightSegment) * new_capacity);
        if (!new_segments) return;  /* Allocation failed, skip this segment */
        hl->segments = new_segments;
        hl->capacity = new_capacity;
    }

    HighlightSegment *seg = &hl->segments[hl->num_segments];
    seg->start = start;
    seg->end = end;
    seg->type = type;
    hl->num_segments++;
}

HighlightedLine *syntax_highlight_line(Syntax *syn, const char *line, bool prev_multiline) {
    HighlightedLine *hl = malloc(sizeof(HighlightedLine));
    if (!hl) return NULL;

    hl->segments = NULL;
    hl->num_segments = 0;
    hl->capacity = 0;
    hl->in_multiline = prev_multiline;

    if (!syn || !line) return hl;

    int len = strlen(line);
    int i = 0;

    /* Handle multiline comments */
    if (prev_multiline && syn->multiline_end) {
        char *end = strstr(line, syn->multiline_end);
        if (end) {
            int end_pos = end - line + strlen(syn->multiline_end);
            add_segment(hl, 0, end_pos, HL_COMMENT);
            i = end_pos;
            hl->in_multiline = false;
        } else {
            /* Entire line is comment */
            add_segment(hl, 0, len, HL_COMMENT);
            return hl;
        }
    }

    while (i < len) {
        /* Skip whitespace */
        if (isspace(line[i])) {
            i++;
            continue;
        }

        /* Check for single-line comment */
        if (syn->singleline_comment) {
            int comment_len = strlen(syn->singleline_comment);
            if (strncmp(&line[i], syn->singleline_comment, comment_len) == 0) {
                add_segment(hl, i, len, HL_COMMENT);
                break;
            }
        }

        /* Check for multi-line comment start */
        if (syn->multiline_start) {
            int start_len = strlen(syn->multiline_start);
            if (strncmp(&line[i], syn->multiline_start, start_len) == 0) {
                char *end = strstr(&line[i + start_len], syn->multiline_end);
                if (end) {
                    int end_pos = end - line + strlen(syn->multiline_end);
                    add_segment(hl, i, end_pos, HL_COMMENT);
                    i = end_pos;
                } else {
                    add_segment(hl, i, len, HL_COMMENT);
                    hl->in_multiline = true;
                    break;
                }
                continue;
            }
        }

        /* Check for string literals */
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            int start = i;
            i++;
            while (i < len && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < len) {
                    i++; /* Skip escaped character */
                }
                i++;
            }
            if (i < len) i++; /* Include closing quote */
            add_segment(hl, start, i, HL_STRING);
            continue;
        }

        /* Check for numbers */
        if (isdigit(line[i])) {
            int start = i;
            while (i < len && (isdigit(line[i]) || line[i] == '.' || line[i] == 'x' ||
                   line[i] == 'X' || (line[i] >= 'a' && line[i] <= 'f') ||
                   (line[i] >= 'A' && line[i] <= 'F'))) {
                i++;
            }
            add_segment(hl, start, i, HL_NUMBER);
            continue;
        }

        /* Check for keywords and identifiers */
        if (isalpha(line[i]) || line[i] == '_') {
            int start = i;
            while (i < len && (isalnum(line[i]) || line[i] == '_')) {
                i++;
            }

            /* Extract word */
            int word_len = i - start;
            char word[256];
            if (word_len < 255) {  /* Leave room for null terminator */
                strncpy(word, &line[start], word_len);
                word[word_len] = '\0';

                /* Check if it's a keyword */
                bool found = false;
                for (size_t r = 0; r < syn->num_rules; r++) {
                    if (syn->rules[r].type == PATTERN_KEYWORD) {
                        if (strcmp(word, syn->rules[r].pattern) == 0) {
                            add_segment(hl, start, i, syn->rules[r].hl_type);
                            found = true;
                            break;
                        }
                    }
                }

                if (!found) {
                    /* Normal identifier - no highlighting */
                }
            }
            continue;
        }

        /* Default: skip character */
        i++;
    }

    return hl;
}

void syntax_free_highlighted_line(HighlightedLine *hl) {
    if (!hl) return;
    if (hl->segments) free(hl->segments);
    free(hl);
}
