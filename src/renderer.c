#define _POSIX_C_SOURCE 200809L
#include "renderer.h"
#include "colors.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/* Terminal renderer backend */
typedef struct {
    Terminal *term;
} TerminalBackend;

/* Detect TrueColor support by checking COLORTERM environment variable */
bool renderer_has_truecolor(void) {
    const char *colorterm = getenv("COLORTERM");
    if (colorterm) {
        if (strcmp(colorterm, "truecolor") == 0 || strcmp(colorterm, "24bit") == 0) {
            return true;
        }
    }

    /* Also check TERM for some known TrueColor terminals */
    const char *term = getenv("TERM");
    if (term) {
        if (strstr(term, "256color") || strstr(term, "24bit")) {
            /* Likely supports TrueColor, though 256color doesn't guarantee it */
            return true;
        }
    }

    return false;
}

/* Check if GPU rendering is available */
bool renderer_has_gpu(void) {
    /* Check if running in a graphical session */
    const char *display = getenv("DISPLAY");
    const char *wayland = getenv("WAYLAND_DISPLAY");

    if (display || wayland) {
        /* We're in a graphical environment */
#ifdef ENABLE_GPU_RENDERER
        return true;
#else
        /* GPU renderer not compiled in */
        return false;
#endif
    }

    return false;
}

RendererConfig renderer_detect_capabilities(void) {
    RendererConfig config = {0};

    config.gpu_available = renderer_has_gpu();
    config.truecolor_enabled = renderer_has_truecolor();

    /* Default to terminal renderer */
    config.type = RENDERER_TERMINAL;

    /* Use GPU renderer if available and user wants it */
    if (config.gpu_available) {
        const char *prefer_gpu = getenv("OCCE_USE_GPU");
        if (prefer_gpu && strcmp(prefer_gpu, "1") == 0) {
            config.type = RENDERER_GPU;
        }
    }

    config.font_size = 14;
    config.font_name = strdup("monospace");

    return config;
}

/* ========== Terminal Renderer Implementation ========== */

static int terminal_renderer_init(Renderer *self) {
    (void)self;  /* Unused parameter */
    /* Terminal is already initialized */
    return 0;
}

static void terminal_renderer_destroy(Renderer *self) {
    TerminalBackend *backend = (TerminalBackend *)self->backend_data;
    if (backend) {
        free(backend);
    }
}

static void terminal_renderer_clear(Renderer *self) {
    TerminalBackend *backend = (TerminalBackend *)self->backend_data;
    terminal_clear(backend->term);
    terminal_write_str(backend->term, "\x1b[2J\x1b[H");
}

static void terminal_renderer_present(Renderer *self) {
    TerminalBackend *backend = (TerminalBackend *)self->backend_data;
    terminal_flush(backend->term);
}

static void terminal_renderer_render_text(Renderer *self, int x, int y, const char *text, HighlightType hl_type) {
    TerminalBackend *backend = (TerminalBackend *)self->backend_data;
    char color_buf[64];

    /* Move cursor */
    terminal_move_cursor(backend->term, y, x);

    /* Set color based on highlight type */
    if (self->config.truecolor_enabled) {
        colors_to_ansi_themed(hl_type, color_buf, sizeof(color_buf));
    } else {
        ColorPair cp = colors_get(hl_type);
        colors_to_ansi(cp, color_buf, sizeof(color_buf));
    }

    terminal_write_str(backend->term, color_buf);
    terminal_write_str(backend->term, text);
    terminal_write_str(backend->term, "\x1b[0m");  /* Reset */
}

static void terminal_renderer_render_line(Renderer *self, int row, const char *line, HighlightedLine *highlights) {
    TerminalBackend *backend = (TerminalBackend *)self->backend_data;
    char color_buf[64];

    terminal_move_cursor(backend->term, row, 0);

    if (!highlights || highlights->num_segments == 0) {
        /* No highlighting, render plain */
        terminal_write_str(backend->term, line);
        return;
    }

    /* Render line with syntax highlighting */
    int line_len = strlen(line);
    int pos = 0;

    for (size_t i = 0; i < highlights->num_segments; i++) {
        HighlightSegment *seg = &highlights->segments[i];

        /* Write any text before this segment (unhighlighted) */
        if (pos < seg->start) {
            int len = seg->start - pos;
            terminal_write(backend->term, line + pos, len);
            pos = seg->start;
        }

        /* Write highlighted segment */
        if (self->config.truecolor_enabled) {
            colors_to_ansi_themed(seg->type, color_buf, sizeof(color_buf));
        } else {
            ColorPair cp = colors_get(seg->type);
            colors_to_ansi(cp, color_buf, sizeof(color_buf));
        }

        terminal_write_str(backend->term, color_buf);
        int seg_len = seg->end - seg->start;
        if (seg_len > 0 && seg->start < line_len) {
            terminal_write(backend->term, line + seg->start, seg_len);
        }
        terminal_write_str(backend->term, "\x1b[0m");  /* Reset colors */

        pos = seg->end;
    }

    /* Write remaining text */
    if (pos < line_len) {
        terminal_write(backend->term, line + pos, line_len - pos);
    }
}

static void terminal_renderer_set_cursor(Renderer *self, int x, int y) {
    TerminalBackend *backend = (TerminalBackend *)self->backend_data;
    terminal_move_cursor(backend->term, y, x);
}

static void terminal_renderer_hide_cursor(Renderer *self) {
    TerminalBackend *backend = (TerminalBackend *)self->backend_data;
    terminal_hide_cursor(backend->term);
}

static void terminal_renderer_show_cursor(Renderer *self) {
    TerminalBackend *backend = (TerminalBackend *)self->backend_data;
    terminal_show_cursor(backend->term);
}

static int terminal_renderer_get_width(Renderer *self) {
    TerminalBackend *backend = (TerminalBackend *)self->backend_data;
    return backend->term->cols;
}

static int terminal_renderer_get_height(Renderer *self) {
    TerminalBackend *backend = (TerminalBackend *)self->backend_data;
    return backend->term->rows;
}

Renderer *renderer_create_terminal(Terminal *term) {
    Renderer *renderer = calloc(1, sizeof(Renderer));
    if (!renderer) return NULL;

    TerminalBackend *backend = calloc(1, sizeof(TerminalBackend));
    if (!backend) {
        free(renderer);
        return NULL;
    }

    backend->term = term;

    renderer->type = RENDERER_TERMINAL;
    renderer->config = renderer_detect_capabilities();
    renderer->backend_data = backend;

    /* Set function pointers */
    renderer->init = terminal_renderer_init;
    renderer->destroy = terminal_renderer_destroy;
    renderer->clear = terminal_renderer_clear;
    renderer->present = terminal_renderer_present;
    renderer->render_text = terminal_renderer_render_text;
    renderer->render_line = terminal_renderer_render_line;
    renderer->set_cursor = terminal_renderer_set_cursor;
    renderer->hide_cursor = terminal_renderer_hide_cursor;
    renderer->show_cursor = terminal_renderer_show_cursor;
    renderer->get_width = terminal_renderer_get_width;
    renderer->get_height = terminal_renderer_get_height;

    return renderer;
}

/* ========== GPU Renderer Implementation ========== */

#ifdef ENABLE_GPU_RENDERER
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>

typedef struct {
    SDL_Window *window;
    SDL_GLContext gl_context;
    int width;
    int height;
    int char_width;
    int char_height;
    /* TODO: Add font rendering with FreeType */
} GPUBackend;

static int gpu_renderer_init(Renderer *self) {
    GPUBackend *backend = (GPUBackend *)self->backend_data;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    /* Set OpenGL attributes */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    /* Create window */
    backend->window = SDL_CreateWindow(
        "OCCE Editor",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        backend->width,
        backend->height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!backend->window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    /* Create OpenGL context */
    backend->gl_context = SDL_GL_CreateContext(backend->window);
    if (!backend->gl_context) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(backend->window);
        SDL_Quit();
        return -1;
    }

    /* Enable VSync */
    SDL_GL_SetSwapInterval(1);

    /* Initialize OpenGL */
    glViewport(0, 0, backend->width, backend->height);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    /* TODO: Initialize font rendering with FreeType */
    backend->char_width = 8;   /* Placeholder */
    backend->char_height = 16; /* Placeholder */

    return 0;
}

static void gpu_renderer_destroy(Renderer *self) {
    GPUBackend *backend = (GPUBackend *)self->backend_data;
    if (backend) {
        if (backend->gl_context) {
            SDL_GL_DeleteContext(backend->gl_context);
        }
        if (backend->window) {
            SDL_DestroyWindow(backend->window);
        }
        SDL_Quit();
        free(backend);
    }
}

static void gpu_renderer_clear(Renderer *self) {
    glClear(GL_COLOR_BUFFER_BIT);
}

static void gpu_renderer_present(Renderer *self) {
    GPUBackend *backend = (GPUBackend *)self->backend_data;
    SDL_GL_SwapWindow(backend->window);
}

static void gpu_renderer_render_text(Renderer *self, int x, int y, const char *text, HighlightType hl_type) {
    /* TODO: Implement GPU text rendering with FreeType and texture atlas */
    /* This would use OpenGL to render glyphs from a texture atlas */
}

static void gpu_renderer_render_line(Renderer *self, int row, const char *line, HighlightedLine *highlights) {
    /* TODO: Implement GPU line rendering */
}

static int gpu_renderer_get_width(Renderer *self) {
    GPUBackend *backend = (GPUBackend *)self->backend_data;
    return backend->width / backend->char_width;
}

static int gpu_renderer_get_height(Renderer *self) {
    GPUBackend *backend = (GPUBackend *)self->backend_data;
    return backend->height / backend->char_height;
}

Renderer *renderer_create_gpu(int width, int height) {
    Renderer *renderer = calloc(1, sizeof(Renderer));
    if (!renderer) return NULL;

    GPUBackend *backend = calloc(1, sizeof(GPUBackend));
    if (!backend) {
        free(renderer);
        return NULL;
    }

    backend->width = width;
    backend->height = height;

    renderer->type = RENDERER_GPU;
    renderer->config = renderer_detect_capabilities();
    renderer->config.type = RENDERER_GPU;
    renderer->backend_data = backend;

    /* Set function pointers */
    renderer->init = gpu_renderer_init;
    renderer->destroy = gpu_renderer_destroy;
    renderer->clear = gpu_renderer_clear;
    renderer->present = gpu_renderer_present;
    renderer->render_text = gpu_renderer_render_text;
    renderer->render_line = gpu_renderer_render_line;
    renderer->set_cursor = NULL;  /* TODO */
    renderer->hide_cursor = NULL; /* TODO */
    renderer->show_cursor = NULL; /* TODO */
    renderer->get_width = gpu_renderer_get_width;
    renderer->get_height = gpu_renderer_get_height;

    return renderer;
}
#endif /* ENABLE_GPU_RENDERER */

/* ========== Generic Renderer Interface ========== */

Renderer *renderer_create(RendererConfig *config) {
    if (!config) {
        RendererConfig default_config = renderer_detect_capabilities();
        config = &default_config;
    }

    if (config->type == RENDERER_GPU && config->gpu_available) {
#ifdef ENABLE_GPU_RENDERER
        return renderer_create_gpu(1024, 768);  /* Default size */
#else
        /* Fall back to terminal */
        fprintf(stderr, "GPU renderer not available, falling back to terminal\n");
#endif
    }

    /* Create terminal renderer */
    Terminal *term = terminal_create();
    if (!term) return NULL;

    return renderer_create_terminal(term);
}

void renderer_destroy(Renderer *renderer) {
    if (!renderer) return;

    if (renderer->destroy) {
        renderer->destroy(renderer);
    }

    if (renderer->config.font_name) {
        free(renderer->config.font_name);
    }

    free(renderer);
}
