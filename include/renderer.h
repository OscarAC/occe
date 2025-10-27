#ifndef RENDERER_H
#define RENDERER_H

#include "terminal.h"
#include "syntax.h"
#include "theme.h"
#include <stdbool.h>

/* Rendering backend types */
typedef enum {
    RENDERER_TERMINAL,    /* Terminal/ANSI escape sequences */
    RENDERER_GPU          /* GPU-accelerated rendering (SDL2 + OpenGL) */
} RendererType;

/* Renderer configuration */
typedef struct {
    RendererType type;
    bool truecolor_enabled;    /* Support 24-bit RGB colors */
    bool gpu_available;        /* GPU acceleration available */
    int font_size;             /* Font size for GPU renderer */
    char *font_name;           /* Font name/path for GPU renderer */
} RendererConfig;

/* Forward declarations */
typedef struct Renderer Renderer;
typedef struct Buffer Buffer;
typedef struct Window Window;

/* Renderer interface */
struct Renderer {
    RendererType type;
    RendererConfig config;

    /* Initialization/cleanup */
    int (*init)(Renderer *self);
    void (*destroy)(Renderer *self);

    /* Rendering functions */
    void (*clear)(Renderer *self);
    void (*present)(Renderer *self);
    void (*render_text)(Renderer *self, int x, int y, const char *text, HighlightType hl_type);
    void (*render_line)(Renderer *self, int row, const char *line, HighlightedLine *highlights);
    void (*set_cursor)(Renderer *self, int x, int y);
    void (*hide_cursor)(Renderer *self);
    void (*show_cursor)(Renderer *self);

    /* Dimension queries */
    int (*get_width)(Renderer *self);
    int (*get_height)(Renderer *self);

    /* Backend-specific data */
    void *backend_data;
};

/* Initialize the renderer system */
Renderer *renderer_create(RendererConfig *config);

/* Destroy renderer */
void renderer_destroy(Renderer *renderer);

/* Detect rendering capabilities */
RendererConfig renderer_detect_capabilities(void);

/* Check if GPU rendering is available */
bool renderer_has_gpu(void);

/* Check if terminal supports TrueColor */
bool renderer_has_truecolor(void);

/* Terminal renderer (ANSI-based) */
Renderer *renderer_create_terminal(Terminal *term);

/* GPU renderer (SDL2 + OpenGL) */
#ifdef ENABLE_GPU_RENDERER
Renderer *renderer_create_gpu(int width, int height);
#endif

#endif /* RENDERER_H */
