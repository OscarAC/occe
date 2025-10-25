#ifndef SYNTAX_H
#define SYNTAX_H

#include "colors.h"
#include <stddef.h>
#include <stdbool.h>

/* Maximum number of syntax rules per language */
#define MAX_SYNTAX_RULES 128
#define MAX_KEYWORDS 256

/* Syntax pattern types */
typedef enum {
    PATTERN_KEYWORD,        /* Exact keyword match */
    PATTERN_MATCH,          /* Lua pattern match */
    PATTERN_MULTILINE_START, /* Start of multiline comment */
    PATTERN_MULTILINE_END    /* End of multiline comment */
} PatternType;

/* A single syntax rule */
typedef struct {
    PatternType type;
    char *pattern;          /* Pattern or keyword */
    HighlightType hl_type;  /* Color to use */
    int priority;           /* Higher priority = checked first */
} SyntaxRule;

/* Syntax definition for a language */
typedef struct Syntax {
    char *name;             /* Language name (e.g., "lua", "c") */
    char **extensions;      /* File extensions (e.g., {".lua", ".luac", NULL}) */
    size_t num_extensions;

    SyntaxRule *rules;      /* Array of syntax rules */
    size_t num_rules;
    size_t rules_capacity;

    char *singleline_comment; /* Single-line comment start (e.g., double slash) */
    char *multiline_start;     /* Multi-line comment start */
    char *multiline_end;       /* Multi-line comment end */

    struct Syntax *next;    /* Linked list of syntaxes */
} Syntax;

/* Highlighted segment in a line */
typedef struct {
    int start;              /* Start column */
    int end;                /* End column */
    HighlightType type;     /* Highlight type */
} HighlightSegment;

/* Highlighted line */
typedef struct HighlightedLine {
    HighlightSegment *segments;
    size_t num_segments;
    size_t capacity;
    bool in_multiline;      /* Is this line part of multiline comment? */
} HighlightedLine;

/* Global syntax registry */
extern Syntax *syntax_list;

/* Initialize syntax system */
void syntax_init(void);

/* Register a new syntax */
Syntax *syntax_register(const char *name);

/* Add file extension to syntax */
void syntax_add_extension(Syntax *syn, const char *ext);

/* Add syntax rule */
void syntax_add_rule(Syntax *syn, PatternType type, const char *pattern, HighlightType hl_type);

/* Add keyword */
void syntax_add_keyword(Syntax *syn, const char *keyword, HighlightType hl_type);

/* Set comment markers */
void syntax_set_comments(Syntax *syn, const char *single, const char *multi_start, const char *multi_end);

/* Find syntax by filename */
Syntax *syntax_find_by_filename(const char *filename);

/* Highlight a line of text */
HighlightedLine *syntax_highlight_line(Syntax *syn, const char *line, bool prev_multiline);

/* Free highlighted line */
void syntax_free_highlighted_line(HighlightedLine *hl);

#endif /* SYNTAX_H */
