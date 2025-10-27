#define _POSIX_C_SOURCE 200809L
#include "lua_bridge.h"
#include "buffer.h"
#include "window.h"
#include "terminal.h"
#include "keybind.h"
#include "search.h"
#include "syntax.h"
#include "colors.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/wait.h>
#include <stdio.h>

/* Helper to get editor from Lua state */
static Editor *get_editor(lua_State *L) {
    lua_getglobal(L, "_EDITOR_PTR");
    Editor *ed = (Editor *)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return ed;
}

/* Lua API: buffer.insert_char(char) */
static int l_buffer_insert_char(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->content.buffer) {
        return luaL_error(L, "No active buffer");
    }

    int c = luaL_checkinteger(L, 1);
    buffer_insert_char(ed->active_window->content.buffer, c);
    return 0;
}

/* Lua API: buffer.insert_string(str) */
static int l_buffer_insert_string(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->content.buffer) {
        return luaL_error(L, "No active buffer");
    }

    size_t len;
    const char *str = luaL_checklstring(L, 1, &len);

    for (size_t i = 0; i < len; i++) {
        buffer_insert_char(ed->active_window->content.buffer, str[i]);
    }
    return 0;
}

/* Lua API: buffer.delete_char() */
static int l_buffer_delete_char(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->content.buffer) {
        return luaL_error(L, "No active buffer");
    }

    buffer_delete_char(ed->active_window->content.buffer);
    return 0;
}

/* Lua API: buffer.insert_newline() */
static int l_buffer_insert_newline(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->content.buffer) {
        return luaL_error(L, "No active buffer");
    }

    buffer_insert_newline(ed->active_window->content.buffer);
    return 0;
}

/* Lua API: buffer.save() */
static int l_buffer_save(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->content.buffer) {
        return luaL_error(L, "No active buffer");
    }

    int result = buffer_save(ed->active_window->content.buffer);
    lua_pushboolean(L, result == 0);
    return 1;
}

/* Lua API: buffer.get_cursor() -> x, y */
static int l_buffer_get_cursor(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->content.buffer) {
        return luaL_error(L, "No active buffer");
    }

    Buffer *buf = ed->active_window->content.buffer;
    lua_pushinteger(L, buf->cursor_x);
    lua_pushinteger(L, buf->cursor_y);
    return 2;
}

/* Lua API: buffer.set_cursor(x, y) */
static int l_buffer_set_cursor(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->content.buffer) {
        return luaL_error(L, "No active buffer");
    }

    Buffer *buf = ed->active_window->content.buffer;
    buf->cursor_x = luaL_checkinteger(L, 1);
    buf->cursor_y = luaL_checkinteger(L, 2);
    return 0;
}

/* Lua API: buffer.get_line(y) -> string */
static int l_buffer_get_line(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->content.buffer) {
        return luaL_error(L, "No active buffer");
    }

    Buffer *buf = ed->active_window->content.buffer;
    int y = luaL_checkinteger(L, 1);

    if (y < 0 || y >= (int)buf->num_rows) {
        lua_pushnil(L);
        return 1;
    }

    BufferRow *row = &buf->rows[y];
    lua_pushlstring(L, row->data, row->size);
    return 1;
}

/* Lua API: buffer.get_line_count() -> count */
static int l_buffer_get_line_count(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->content.buffer) {
        return luaL_error(L, "No active buffer");
    }

    Buffer *buf = ed->active_window->content.buffer;
    lua_pushinteger(L, buf->num_rows);
    return 1;
}

/* Lua API: buffer.get_char(x, y) -> char or nil */
static int l_buffer_get_char(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->content.buffer) {
        return luaL_error(L, "No active buffer");
    }

    Buffer *buf = ed->active_window->content.buffer;
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);

    if (y < 0 || y >= (int)buf->num_rows) {
        lua_pushnil(L);
        return 1;
    }

    BufferRow *row = &buf->rows[y];
    if (x < 0 || x >= (int)row->size) {
        lua_pushnil(L);
        return 1;
    }

    char ch[2] = {row->data[x], '\0'};
    lua_pushstring(L, ch);
    return 1;
}

/* Lua API: buffer.get_line_length(y) -> length */
static int l_buffer_get_line_length(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->content.buffer) {
        return luaL_error(L, "No active buffer");
    }

    Buffer *buf = ed->active_window->content.buffer;
    int y = luaL_checkinteger(L, 1);

    if (y < 0 || y >= (int)buf->num_rows) {
        lua_pushinteger(L, 0);
        return 1;
    }

    BufferRow *row = &buf->rows[y];
    lua_pushinteger(L, row->size);
    return 1;
}

/* Lua API: buffer.get_filename() -> string or nil */
static int l_buffer_get_filename(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->content.buffer) {
        return luaL_error(L, "No active buffer");
    }

    Buffer *buf = ed->active_window->content.buffer;
    if (buf->filename) {
        lua_pushstring(L, buf->filename);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

/* Lua API: buffer.search(query, forward) -> row, col or nil */
static int l_buffer_search(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->content.buffer) {
        return luaL_error(L, "No active buffer");
    }

    Buffer *buf = ed->active_window->content.buffer;
    const char *query = luaL_checkstring(L, 1);
    bool forward = lua_toboolean(L, 2);

    SearchResult *result = buffer_search(buf, query, buf->cursor_y, buf->cursor_x, forward);

    if (result) {
        lua_pushinteger(L, result->row);
        lua_pushinteger(L, result->col);
        free(result);
        return 2;
    }

    lua_pushnil(L);
    return 1;
}

/* Lua API: buffer.replace(search, replace, all) -> count */
static int l_buffer_replace(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->content.buffer) {
        return luaL_error(L, "No active buffer");
    }

    Buffer *buf = ed->active_window->content.buffer;
    const char *search = luaL_checkstring(L, 1);
    const char *replace = luaL_checkstring(L, 2);
    bool all = lua_toboolean(L, 3);

    int count = buffer_replace(buf, search, replace, all);
    lua_pushinteger(L, count);
    return 1;
}

/* Lua API: editor.quit() */
static int l_editor_quit(lua_State *L) {
    Editor *ed = get_editor(L);
    if (ed) {
        editor_quit(ed);
    }
    return 0;
}

/* Lua API: editor.message(str) - Display a message */
static int l_editor_message(lua_State *L) {
    Editor *ed = get_editor(L);
    const char *msg = luaL_checkstring(L, 1);
    if (ed) {
        editor_set_status(ed, msg);
    }
    return 0;
}

/* Lua API: editor.bind_key(key, modifiers, function_name) */
static int l_editor_bind_key(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->keymap) {
        return luaL_error(L, "No editor");
    }

    int key = luaL_checkinteger(L, 1);
    int modifiers = luaL_optinteger(L, 2, KMOD_NONE);
    const char *func_name = luaL_checkstring(L, 3);

    fprintf(stderr, "DEBUG: Binding key=%d mod=%d func=%s\n", key, modifiers, func_name);
    keymap_bind(ed->keymap, key, modifiers, func_name);
    return 0;
}

/* Lua API: editor.unbind_key(key, modifiers) */
static int l_editor_unbind_key(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->keymap) {
        return luaL_error(L, "No editor");
    }

    int key = luaL_checkinteger(L, 1);
    int modifiers = luaL_optinteger(L, 2, KMOD_NONE);

    keymap_unbind(ed->keymap, key, modifiers);
    return 0;
}

/* Lua API: editor.load_plugin(filename) - Load plugin with dev mode support */
static int l_editor_load_plugin(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed) {
        return luaL_error(L, "No editor");
    }

    const char *filename = luaL_checkstring(L, 1);
    char plugin_path[512];
    int result = -1;
    char last_error[512] = "Plugin file not found";

    /* Try 1: Local plugins directory (for development) */
    snprintf(plugin_path, sizeof(plugin_path), "./plugins/%s", filename);
    result = lua_bridge_load_plugin(ed, plugin_path);

    /* Get error message if failed */
    if (result != 0) {
        lua_getglobal(L, "_PLUGIN_LOAD_ERROR");
        if (lua_isstring(L, -1)) {
            const char *err = lua_tostring(L, -1);
            snprintf(last_error, sizeof(last_error), "%s", err);
        }
        lua_pop(L, 1);
    }

    /* Try 2: User config directory */
    if (result != 0 && ed->config_dir) {
        snprintf(plugin_path, sizeof(plugin_path), "%s/plugins/%s", ed->config_dir, filename);
        result = lua_bridge_load_plugin(ed, plugin_path);

        /* Get error message if failed */
        if (result != 0) {
            lua_getglobal(L, "_PLUGIN_LOAD_ERROR");
            if (lua_isstring(L, -1)) {
                const char *err = lua_tostring(L, -1);
                snprintf(last_error, sizeof(last_error), "%s", err);
            }
            lua_pop(L, 1);
        }
    }

    if (result == 0) {
        lua_pushboolean(L, 1);
        lua_pushnil(L);  /* No error */
        return 2;
    } else {
        lua_pushboolean(L, 0);
        lua_pushstring(L, last_error);
        return 2;
    }
}

/* Lua API: process.execute(command) -> stdout, stderr, exitcode */
static int l_process_execute(lua_State *L) {
    const char *cmd = luaL_checkstring(L, 1);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to execute command");
        lua_pushinteger(L, -1);
        return 3;
    }

    /* Read output */
    char *result = NULL;
    size_t size = 0;
    size_t capacity = 1024;
    result = malloc(capacity);
    if (!result) {
        pclose(fp);
        lua_pushnil(L);
        lua_pushstring(L, "Out of memory");
        lua_pushinteger(L, -1);
        return 3;
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
                lua_pushnil(L);
                lua_pushstring(L, "Out of memory");
                lua_pushinteger(L, -1);
                return 3;
            }
            result = new_result;
        }
        memcpy(result + size, buffer, len);
        size += len;
    }

    int status = pclose(fp);
    result[size] = '\0';

    lua_pushlstring(L, result, size);
    lua_pushstring(L, "");  /* stderr - not separated in popen */
    lua_pushinteger(L, WEXITSTATUS(status));

    free(result);
    return 3;
}

/* Register buffer API functions */
static void register_buffer_api(lua_State *L) {
    lua_newtable(L);

    lua_pushcfunction(L, l_buffer_insert_char);
    lua_setfield(L, -2, "insert_char");

    lua_pushcfunction(L, l_buffer_insert_string);
    lua_setfield(L, -2, "insert_string");

    lua_pushcfunction(L, l_buffer_delete_char);
    lua_setfield(L, -2, "delete_char");

    lua_pushcfunction(L, l_buffer_insert_newline);
    lua_setfield(L, -2, "insert_newline");

    lua_pushcfunction(L, l_buffer_save);
    lua_setfield(L, -2, "save");

    lua_pushcfunction(L, l_buffer_get_cursor);
    lua_setfield(L, -2, "get_cursor");

    lua_pushcfunction(L, l_buffer_set_cursor);
    lua_setfield(L, -2, "set_cursor");

    lua_pushcfunction(L, l_buffer_get_line);
    lua_setfield(L, -2, "get_line");

    lua_pushcfunction(L, l_buffer_get_line_count);
    lua_setfield(L, -2, "get_line_count");

    lua_pushcfunction(L, l_buffer_get_char);
    lua_setfield(L, -2, "get_char");

    lua_pushcfunction(L, l_buffer_get_line_length);
    lua_setfield(L, -2, "get_line_length");

    lua_pushcfunction(L, l_buffer_get_filename);
    lua_setfield(L, -2, "get_filename");

    lua_pushcfunction(L, l_buffer_search);
    lua_setfield(L, -2, "search");

    lua_pushcfunction(L, l_buffer_replace);
    lua_setfield(L, -2, "replace");

    lua_setglobal(L, "buffer");
}

/* Register editor API functions */
static void register_editor_api(lua_State *L) {
    lua_newtable(L);

    lua_pushcfunction(L, l_editor_quit);
    lua_setfield(L, -2, "quit");

    lua_pushcfunction(L, l_editor_message);
    lua_setfield(L, -2, "message");

    lua_pushcfunction(L, l_editor_bind_key);
    lua_setfield(L, -2, "bind_key");

    lua_pushcfunction(L, l_editor_unbind_key);
    lua_setfield(L, -2, "unbind_key");

    lua_pushcfunction(L, l_editor_load_plugin);
    lua_setfield(L, -2, "load_plugin");

    /* Key modifier constants */
    lua_newtable(L);
    lua_pushinteger(L, KMOD_NONE);
    lua_setfield(L, -2, "NONE");
    lua_pushinteger(L, KMOD_CTRL);
    lua_setfield(L, -2, "CTRL");
    lua_pushinteger(L, KMOD_ALT);
    lua_setfield(L, -2, "ALT");
    lua_pushinteger(L, KMOD_SHIFT);
    lua_setfield(L, -2, "SHIFT");
    lua_setfield(L, -2, "KMOD");

    /* Key code constants */
    lua_newtable(L);
    lua_pushinteger(L, KEY_CTRL_ARROW_LEFT);
    lua_setfield(L, -2, "CTRL_ARROW_LEFT");
    lua_pushinteger(L, KEY_CTRL_ARROW_RIGHT);
    lua_setfield(L, -2, "CTRL_ARROW_RIGHT");
    lua_pushinteger(L, KEY_CTRL_ARROW_UP);
    lua_setfield(L, -2, "CTRL_ARROW_UP");
    lua_pushinteger(L, KEY_CTRL_ARROW_DOWN);
    lua_setfield(L, -2, "CTRL_ARROW_DOWN");
    lua_pushinteger(L, KEY_ARROW_LEFT);
    lua_setfield(L, -2, "ARROW_LEFT");
    lua_pushinteger(L, KEY_ARROW_RIGHT);
    lua_setfield(L, -2, "ARROW_RIGHT");
    lua_pushinteger(L, KEY_ARROW_UP);
    lua_setfield(L, -2, "ARROW_UP");
    lua_pushinteger(L, KEY_ARROW_DOWN);
    lua_setfield(L, -2, "ARROW_DOWN");
    lua_setfield(L, -2, "KEY");

    lua_setglobal(L, "editor");
}

/* Lua API: syntax.register(name) -> syntax object */
static int l_syntax_register(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    Syntax *syn = syntax_register(name);

    if (syn) {
        lua_pushlightuserdata(L, syn);
        return 1;
    }

    return 0;
}

/* Lua API: syntax.add_extension(syn, ext) */
static int l_syntax_add_extension(lua_State *L) {
    Syntax *syn = (Syntax *)lua_touserdata(L, 1);
    const char *ext = luaL_checkstring(L, 2);

    if (syn && ext) {
        syntax_add_extension(syn, ext);
    }

    return 0;
}

/* Lua API: syntax.add_keyword(syn, keyword, hl_type) */
static int l_syntax_add_keyword(lua_State *L) {
    Syntax *syn = (Syntax *)lua_touserdata(L, 1);
    const char *keyword = luaL_checkstring(L, 2);
    int hl_type = luaL_checkinteger(L, 3);

    if (syn && keyword) {
        syntax_add_keyword(syn, keyword, (HighlightType)hl_type);
    }

    return 0;
}

/* Lua API: syntax.set_comments(syn, single, multi_start, multi_end) */
static int l_syntax_set_comments(lua_State *L) {
    Syntax *syn = (Syntax *)lua_touserdata(L, 1);
    const char *single = lua_isnil(L, 2) ? NULL : lua_tostring(L, 2);
    const char *multi_start = lua_isnil(L, 3) ? NULL : lua_tostring(L, 3);
    const char *multi_end = lua_isnil(L, 4) ? NULL : lua_tostring(L, 4);

    if (syn) {
        syntax_set_comments(syn, single, multi_start, multi_end);
    }

    return 0;
}

/* Register syntax API functions */
static void register_syntax_api(lua_State *L) {
    lua_newtable(L);

    lua_pushcfunction(L, l_syntax_register);
    lua_setfield(L, -2, "register");

    lua_pushcfunction(L, l_syntax_add_extension);
    lua_setfield(L, -2, "add_extension");

    lua_pushcfunction(L, l_syntax_add_keyword);
    lua_setfield(L, -2, "add_keyword");

    lua_pushcfunction(L, l_syntax_set_comments);
    lua_setfield(L, -2, "set_comments");

    /* Highlight type constants */
    lua_pushinteger(L, HL_NORMAL);
    lua_setfield(L, -2, "HL_NORMAL");
    lua_pushinteger(L, HL_KEYWORD);
    lua_setfield(L, -2, "HL_KEYWORD");
    lua_pushinteger(L, HL_TYPE);
    lua_setfield(L, -2, "HL_TYPE");
    lua_pushinteger(L, HL_STRING);
    lua_setfield(L, -2, "HL_STRING");
    lua_pushinteger(L, HL_NUMBER);
    lua_setfield(L, -2, "HL_NUMBER");
    lua_pushinteger(L, HL_COMMENT);
    lua_setfield(L, -2, "HL_COMMENT");
    lua_pushinteger(L, HL_OPERATOR);
    lua_setfield(L, -2, "HL_OPERATOR");
    lua_pushinteger(L, HL_FUNCTION);
    lua_setfield(L, -2, "HL_FUNCTION");
    lua_pushinteger(L, HL_VARIABLE);
    lua_setfield(L, -2, "HL_VARIABLE");
    lua_pushinteger(L, HL_CONSTANT);
    lua_setfield(L, -2, "HL_CONSTANT");
    lua_pushinteger(L, HL_PREPROCESSOR);
    lua_setfield(L, -2, "HL_PREPROCESSOR");

    lua_setglobal(L, "syntax");
}

/* Register process API functions */
static void register_process_api(lua_State *L) {
    lua_newtable(L);

    lua_pushcfunction(L, l_process_execute);
    lua_setfield(L, -2, "execute");

    lua_setglobal(L, "process");
}

/* ===== Terminal API Functions ===== */

/* Lua API: terminal.move(row, col) */
static int l_terminal_move(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->term) return 0;

    int row = luaL_checkinteger(L, 1);
    int col = luaL_checkinteger(L, 2);

    terminal_move_cursor(ed->term, row, col);
    return 0;
}

/* Lua API: terminal.write(text) */
static int l_terminal_write(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->term) return 0;

    size_t len;
    const char *text = luaL_checklstring(L, 1, &len);

    terminal_write(ed->term, text, len);
    return 0;
}

/* Lua API: terminal.clear_line() */
static int l_terminal_clear_line(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->term) return 0;

    terminal_write_str(ed->term, "\x1b[K");
    return 0;
}

/* Lua API: terminal.set_color(fg, bg) - ANSI color codes */
static int l_terminal_set_color(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->term) return 0;

    const char *code = luaL_checkstring(L, 1);
    terminal_write_str(ed->term, code);
    return 0;
}

/* Lua API: terminal.reset_color() */
static int l_terminal_reset_color(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->term) return 0;

    terminal_write_str(ed->term, "\x1b[0m");
    return 0;
}

/* Register terminal API */
static void register_terminal_api(lua_State *L) {
    lua_newtable(L);

    lua_pushcfunction(L, l_terminal_move);
    lua_setfield(L, -2, "move");

    lua_pushcfunction(L, l_terminal_write);
    lua_setfield(L, -2, "write");

    lua_pushcfunction(L, l_terminal_clear_line);
    lua_setfield(L, -2, "clear_line");

    lua_pushcfunction(L, l_terminal_set_color);
    lua_setfield(L, -2, "set_color");

    lua_pushcfunction(L, l_terminal_reset_color);
    lua_setfield(L, -2, "reset_color");

    lua_setglobal(L, "terminal");
}

/* ===== Window API Functions ===== */

/* Lua API: window.get_current() -> window_id */
static int l_window_get_current(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushinteger(L, ed->active_window->id);
    return 1;
}

/* Lua API: window.get_all() -> {window_id, ...} */
static int l_window_get_all(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->root_window) {
        lua_newtable(L);
        return 1;
    }

    /* Collect all leaf windows */
    Window *leaves[256];
    size_t count = 0;

    /* Helper to collect leaves - we'll use the existing window_collect_leaves logic */
    /* For now, just return the current window */
    lua_newtable(L);
    lua_pushinteger(L, ed->active_window->id);
    lua_rawseti(L, -2, 1);

    return 1;
}

/* Lua API: window.get_info(win_id) -> {x, y, width, height, type, id, focused} */
static int l_window_get_info(lua_State *L) {
    Editor *ed = get_editor(L);
    int win_id = luaL_optinteger(L, 1, -1);

    Window *win = NULL;
    if (win_id == -1 || win_id == 0) {
        win = ed->active_window;
    } else {
        win = window_find_by_id(ed->root_window, win_id);
    }

    if (!win) {
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);
    lua_pushinteger(L, win->x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, win->y);
    lua_setfield(L, -2, "y");
    lua_pushinteger(L, win->width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, win->height);
    lua_setfield(L, -2, "height");
    lua_pushinteger(L, win->id);
    lua_setfield(L, -2, "id");
    lua_pushboolean(L, win->focused);
    lua_setfield(L, -2, "focused");

    const char *type_str = "unknown";
    if (win->type == WINDOW_LEAF) type_str = "leaf";
    else if (win->type == WINDOW_SPLIT_H) type_str = "split_h";
    else if (win->type == WINDOW_SPLIT_V) type_str = "split_v";
    lua_pushstring(L, type_str);
    lua_setfield(L, -2, "type");

    return 1;
}

/* Lua API: window.next() */
static int l_window_next(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->root_window || !ed->active_window) {
        return 0;
    }

    Window *next = window_get_next_leaf(ed->root_window, ed->active_window);
    if (next && next != ed->active_window) {
        window_set_focused(ed->active_window, false);
        ed->active_window = next;
        window_set_focused(ed->active_window, true);
        if (ed->active_tab) {
            ed->active_tab->active_window = next;
        }
    }

    return 0;
}

/* Lua API: window.prev() */
static int l_window_prev(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->root_window || !ed->active_window) {
        return 0;
    }

    Window *prev = window_get_prev_leaf(ed->root_window, ed->active_window);
    if (prev && prev != ed->active_window) {
        window_set_focused(ed->active_window, false);
        ed->active_window = prev;
        window_set_focused(ed->active_window, true);
        if (ed->active_tab) {
            ed->active_tab->active_window = prev;
        }
    }

    return 0;
}

/* Lua API: window.focus(win_id) */
static int l_window_focus(lua_State *L) {
    Editor *ed = get_editor(L);
    int win_id = luaL_checkinteger(L, 1);

    if (!ed || !ed->root_window) {
        return 0;
    }

    Window *win = window_find_by_id(ed->root_window, win_id);
    if (win && win->type == WINDOW_LEAF) {
        window_set_focused(ed->active_window, false);
        ed->active_window = win;
        window_set_focused(ed->active_window, true);
        if (ed->active_tab) {
            ed->active_tab->active_window = win;
        }
    }

    return 0;
}

/* Lua API: window.focus_direction(direction) */
static int l_window_focus_direction(lua_State *L) {
    Editor *ed = get_editor(L);
    const char *direction = luaL_checkstring(L, 1);

    if (!ed || !ed->root_window || !ed->active_window) {
        return 0;
    }

    Window *target = window_get_direction(ed->root_window, ed->active_window, direction);
    if (target && target != ed->active_window) {
        window_set_focused(ed->active_window, false);
        ed->active_window = target;
        window_set_focused(ed->active_window, true);
        if (ed->active_tab) {
            ed->active_tab->active_window = target;
        }
    }

    return 0;
}

/* Lua API: window.close(win_id) - nil = current window */
static int l_window_close(lua_State *L) {
    Editor *ed = get_editor(L);
    int win_id = luaL_optinteger(L, 1, -1);

    if (!ed || !ed->root_window) {
        return 0;
    }

    Window *to_close = NULL;
    if (win_id == -1) {
        to_close = ed->active_window;
    } else {
        to_close = window_find_by_id(ed->root_window, win_id);
    }

    if (to_close) {
        Window *new_active = NULL;
        ed->root_window = window_close_split(ed->root_window, to_close, &new_active);
        if (new_active) {
            window_set_focused(ed->active_window, false);
            ed->active_window = new_active;
            window_set_focused(ed->active_window, true);
        }
        if (ed->active_tab) {
            ed->active_tab->root_window = ed->root_window;
            ed->active_tab->active_window = ed->active_window;
        }
    }

    return 0;
}

/* Lua API: window.only() - close all except current */
static int l_window_only(lua_State *L) {
    Editor *ed = get_editor(L);

    if (!ed || !ed->root_window || !ed->active_window) {
        return 0;
    }

    ed->root_window = window_only(ed->root_window, ed->active_window);
    if (ed->active_tab) {
        ed->active_tab->root_window = ed->root_window;
    }

    return 0;
}

/* Lua API: window.swap(win_id_1, win_id_2) */
static int l_window_swap(lua_State *L) {
    Editor *ed = get_editor(L);
    int win_id_1 = luaL_checkinteger(L, 1);
    int win_id_2 = luaL_checkinteger(L, 2);

    if (!ed || !ed->root_window) {
        return 0;
    }

    Window *win1 = window_find_by_id(ed->root_window, win_id_1);
    Window *win2 = window_find_by_id(ed->root_window, win_id_2);

    if (win1 && win2) {
        window_swap(win1, win2);
    }

    return 0;
}

/* Lua API: window.set_split_ratio(win_id, ratio) */
static int l_window_set_split_ratio(lua_State *L) {
    Editor *ed = get_editor(L);
    int win_id = luaL_checkinteger(L, 1);
    float ratio = (float)luaL_checknumber(L, 2);

    if (!ed || !ed->root_window) {
        return 0;
    }

    Window *win = window_find_by_id(ed->root_window, win_id);
    if (win) {
        window_set_split_ratio(win, ratio);
        /* Recalculate layout */
        window_resize(ed->root_window, 0, 0, ed->term->cols, ed->term->rows - 1);
    }

    return 0;
}

/* Lua API: window.register_renderer(name, renderer_table) */
static int l_window_register_renderer(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    /* Get or create _window_renderers global table */
    lua_getglobal(L, "_window_renderers");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);  /* Duplicate table */
        lua_setglobal(L, "_window_renderers");
    }

    /* Store renderer: _window_renderers[name] = renderer_table */
    lua_pushvalue(L, 2);  /* Push renderer table */
    lua_setfield(L, -2, name);

    lua_pop(L, 1);  /* Pop _window_renderers */
    return 0;
}

/* Lua API: window.create_custom(renderer_name, data_table) -> window_id */
static int l_window_create_custom(lua_State *L) {
    Editor *ed = get_editor(L);
    const char *renderer_name = luaL_checkstring(L, 1);

    if (!ed) {
        lua_pushnil(L);
        return 1;
    }

    /* Get data table (optional) */
    int data_ref = LUA_NOREF;
    if (lua_istable(L, 2)) {
        lua_pushvalue(L, 2);  /* Duplicate table */
        data_ref = luaL_ref(L, LUA_REGISTRYINDEX);  /* Store in registry */
    }

    /* Create custom window */
    Window *win = window_create_leaf_custom(renderer_name, 0, 0,
                                            ed->term->cols, ed->term->rows - 1);
    if (!win) {
        if (data_ref != LUA_NOREF) {
            luaL_unref(L, LUA_REGISTRYINDEX, data_ref);
        }
        lua_pushnil(L);
        return 1;
    }

    win->id = ed->next_window_id++;
    win->content.custom_data = (void*)(intptr_t)data_ref;

    /* Replace current window with custom window */
    /* For now, just replace the root window - TODO: support splits */
    if (ed->root_window) {
        window_destroy(ed->root_window);
    }
    ed->root_window = win;
    ed->active_window = win;
    if (ed->active_tab) {
        ed->active_tab->root_window = win;
        ed->active_tab->active_window = win;
    }

    window_resize(ed->root_window, 0, 0, ed->term->cols, ed->term->rows - 1);

    lua_pushinteger(L, win->id);
    return 1;
}

/* Lua API: window.equalize() - Make all windows equal size */
static int l_window_equalize(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->root_window) {
        return 0;
    }

    window_equalize_sizes(ed->root_window);
    window_resize(ed->root_window, 0, 0, ed->term->cols, ed->term->rows - 1);

    return 0;
}

/* Lua API: window.resize_relative(win_id, delta_width, delta_height) */
static int l_window_resize_relative(lua_State *L) {
    Editor *ed = get_editor(L);
    int win_id = luaL_checkinteger(L, 1);
    int delta_w = luaL_checkinteger(L, 2);
    int delta_h = luaL_checkinteger(L, 3);

    if (!ed || !ed->root_window) {
        return 0;
    }

    Window *win = window_find_by_id(ed->root_window, win_id);
    if (!win) {
        return 0;
    }

    /* Find parent to adjust split ratio */
    Window *parent = window_find_parent(ed->root_window, win);
    if (!parent) {
        return 0;  /* Can't resize root */
    }

    /* Adjust split ratio based on delta */
    float delta_ratio = 0.0f;
    if (parent->type == WINDOW_SPLIT_H) {
        delta_ratio = (float)delta_h / (float)parent->height;
    } else if (parent->type == WINDOW_SPLIT_V) {
        delta_ratio = (float)delta_w / (float)parent->width;
    }

    /* Apply delta (if window is on left/top, increase ratio, otherwise decrease) */
    if (parent->left == win) {
        parent->split_ratio += delta_ratio;
    } else {
        parent->split_ratio -= delta_ratio;
    }

    /* Clamp ratio */
    if (parent->split_ratio < 0.1f) parent->split_ratio = 0.1f;
    if (parent->split_ratio > 0.9f) parent->split_ratio = 0.9f;

    /* Recalculate layout */
    window_resize(ed->root_window, 0, 0, ed->term->cols, ed->term->rows - 1);

    return 0;
}

/* Lua API: window.move(direction) - Swap current window with neighbor */
static int l_window_move(lua_State *L) {
    Editor *ed = get_editor(L);
    const char *direction = luaL_checkstring(L, 1);

    if (!ed || !ed->root_window || !ed->active_window) {
        return 0;
    }

    /* Find window in direction */
    Window *target = window_get_direction(ed->root_window, ed->active_window, direction);

    if (target && target != ed->active_window) {
        window_swap(ed->active_window, target);
    }

    return 0;
}

/* Lua API: window.get_count() -> number of leaf windows */
static int l_window_get_count(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->root_window) {
        lua_pushinteger(L, 0);
        return 1;
    }

    int count = window_count_leaves(ed->root_window);
    lua_pushinteger(L, count);
    return 1;
}

/* Lua API: window.register_layout(name, layout_function) */
static int l_window_register_layout(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    /* Get or create _window_layouts global table */
    lua_getglobal(L, "_window_layouts");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);  /* Duplicate table */
        lua_setglobal(L, "_window_layouts");
    }

    /* Store layout: _window_layouts[name] = layout_function */
    lua_pushvalue(L, 2);  /* Push layout function */
    lua_setfield(L, -2, name);

    lua_pop(L, 1);  /* Pop _window_layouts */
    return 0;
}

/* Lua API: window.apply_layout(name) */
static int l_window_apply_layout(lua_State *L) {
    Editor *ed = get_editor(L);
    const char *name = luaL_checkstring(L, 1);

    if (!ed || !ed->root_window) {
        return 0;
    }

    /* Get layout function: _window_layouts[name] */
    lua_getglobal(L, "_window_layouts");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }

    lua_getfield(L, -1, name);
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2);
        return 0;
    }

    /* Collect all windows */
    Window *leaves[256];
    int count = 0;
    window_collect_all_leaves(ed->root_window, leaves, &count, 256);

    /* Create table of window IDs */
    lua_newtable(L);
    for (int i = 0; i < count; i++) {
        lua_pushinteger(L, leaves[i]->id);
        lua_rawseti(L, -2, i + 1);
    }

    /* Push total dimensions */
    lua_pushinteger(L, ed->term->cols);
    lua_pushinteger(L, ed->term->rows - 1);

    /* Call layout_function(windows, total_w, total_h) -> layout_table */
    if (lua_pcall(L, 3, 1, 0) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        (void)err;  /* TODO: Display error */
        lua_pop(L, 2);
        return 0;
    }

    /* Apply the returned layout (table of {id, x, y, w, h}) */
    /* For now, just acknowledge it worked */
    /* TODO: Actually apply the layout positions */

    lua_pop(L, 2);  /* Pop result and _window_layouts */
    return 0;
}

/* ===== Window Event System ===== */

/* Lua API: window.on_create(callback) - Register window creation hook */
static int l_window_on_create(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    /* Get or create _window_hooks.create table */
    lua_getglobal(L, "_window_hooks");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setglobal(L, "_window_hooks");
    }

    lua_getfield(L, -1, "create");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, "create");
    }

    /* Add callback to the table */
    int len = lua_rawlen(L, -1);
    lua_pushvalue(L, 1);  /* Push callback */
    lua_rawseti(L, -2, len + 1);

    lua_pop(L, 2);  /* Pop create table and _window_hooks */
    return 0;
}

/* Lua API: window.on_focus(callback) - Register window focus hook */
static int l_window_on_focus(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    lua_getglobal(L, "_window_hooks");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setglobal(L, "_window_hooks");
    }

    lua_getfield(L, -1, "focus");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, "focus");
    }

    int len = lua_rawlen(L, -1);
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, len + 1);

    lua_pop(L, 2);
    return 0;
}

/* Lua API: window.on_close(callback) - Register window close hook */
static int l_window_on_close(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    lua_getglobal(L, "_window_hooks");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setglobal(L, "_window_hooks");
    }

    lua_getfield(L, -1, "close");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, "close");
    }

    int len = lua_rawlen(L, -1);
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, len + 1);

    lua_pop(L, 2);
    return 0;
}

/* Lua API: window.on_resize(callback) - Register window resize hook */
static int l_window_on_resize(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    lua_getglobal(L, "_window_hooks");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setglobal(L, "_window_hooks");
    }

    lua_getfield(L, -1, "resize");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, "resize");
    }

    int len = lua_rawlen(L, -1);
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, len + 1);

    lua_pop(L, 2);
    return 0;
}

/* ===== Session Management ===== */

/* Lua API: window.serialize() -> session_table */
static int l_window_serialize(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->root_window) {
        lua_newtable(L);
        return 1;
    }

    /* Create session table */
    lua_newtable(L);

    /* Collect all windows */
    Window *leaves[256];
    int count = 0;
    window_collect_all_leaves(ed->root_window, leaves, &count, 256);

    /* Store window count */
    lua_pushinteger(L, count);
    lua_setfield(L, -2, "window_count");

    /* Store active window ID */
    if (ed->active_window) {
        lua_pushinteger(L, ed->active_window->id);
        lua_setfield(L, -2, "active_window_id");
    }

    /* Create windows array */
    lua_newtable(L);
    for (int i = 0; i < count; i++) {
        Window *win = leaves[i];
        lua_newtable(L);

        lua_pushinteger(L, win->id);
        lua_setfield(L, -2, "id");

        lua_pushinteger(L, win->x);
        lua_setfield(L, -2, "x");

        lua_pushinteger(L, win->y);
        lua_setfield(L, -2, "y");

        lua_pushinteger(L, win->width);
        lua_setfield(L, -2, "width");

        lua_pushinteger(L, win->height);
        lua_setfield(L, -2, "height");

        /* Store buffer filename if available */
        if (win->content_type == CONTENT_BUFFER && win->content.buffer) {
            if (win->content.buffer->filename) {
                lua_pushstring(L, win->content.buffer->filename);
                lua_setfield(L, -2, "filename");
            }
        } else if (win->content_type == CONTENT_CUSTOM && win->renderer_name) {
            lua_pushstring(L, win->renderer_name);
            lua_setfield(L, -2, "renderer");
        }

        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "windows");

    return 1;
}

/* Lua API: window.save_session(filename) */
static int l_window_save_session(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);

    /* Serialize window state */
    l_window_serialize(L);

    /* Convert table to Lua code */
    lua_getglobal(L, "string");
    lua_getfield(L, -1, "format");

    /* Build Lua code to recreate session */
    lua_pushstring(L, "return ");

    /* For now, just serialize as a simple return statement */
    /* TODO: Implement proper table serialization */

    lua_pop(L, 2);  /* Pop string and format */

    /* Write to file */
    FILE *f = fopen(filename, "w");
    if (!f) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Failed to open file");
        return 2;
    }

    fprintf(f, "-- OCCE Window Session\n");
    fprintf(f, "-- Saved: %s\n", "TODO: timestamp");
    fprintf(f, "return {}\n");  /* TODO: Implement proper serialization */

    fclose(f);

    lua_pushboolean(L, 1);
    return 1;
}

/* Lua API: window.restore_session(filename) */
static int l_window_restore_session(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);

    /* Load session file */
    if (luaL_dofile((lua_State*)L, filename) != LUA_OK) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Failed to load session file");
        return 2;
    }

    /* Session table should be on top of stack */
    if (!lua_istable(L, -1)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Invalid session file");
        return 2;
    }

    /* TODO: Restore windows from session table */

    lua_pushboolean(L, 1);
    return 1;
}

/* Register window API functions */
static void register_window_api(lua_State *L) {
    lua_newtable(L);

    /* Window queries */
    lua_pushcfunction(L, l_window_get_current);
    lua_setfield(L, -2, "get_current");

    lua_pushcfunction(L, l_window_get_all);
    lua_setfield(L, -2, "get_all");

    lua_pushcfunction(L, l_window_get_info);
    lua_setfield(L, -2, "get_info");

    /* Window navigation */
    lua_pushcfunction(L, l_window_next);
    lua_setfield(L, -2, "next");

    lua_pushcfunction(L, l_window_prev);
    lua_setfield(L, -2, "prev");

    lua_pushcfunction(L, l_window_focus);
    lua_setfield(L, -2, "focus");

    lua_pushcfunction(L, l_window_focus_direction);
    lua_setfield(L, -2, "focus_direction");

    /* Window management */
    lua_pushcfunction(L, l_window_close);
    lua_setfield(L, -2, "close");

    lua_pushcfunction(L, l_window_only);
    lua_setfield(L, -2, "only");

    lua_pushcfunction(L, l_window_swap);
    lua_setfield(L, -2, "swap");

    lua_pushcfunction(L, l_window_set_split_ratio);
    lua_setfield(L, -2, "set_split_ratio");

    /* Custom windows */
    lua_pushcfunction(L, l_window_register_renderer);
    lua_setfield(L, -2, "register_renderer");

    lua_pushcfunction(L, l_window_create_custom);
    lua_setfield(L, -2, "create_custom");

    /* Advanced layout */
    lua_pushcfunction(L, l_window_equalize);
    lua_setfield(L, -2, "equalize");

    lua_pushcfunction(L, l_window_resize_relative);
    lua_setfield(L, -2, "resize_relative");

    lua_pushcfunction(L, l_window_move);
    lua_setfield(L, -2, "move");

    lua_pushcfunction(L, l_window_get_count);
    lua_setfield(L, -2, "get_count");

    lua_pushcfunction(L, l_window_register_layout);
    lua_setfield(L, -2, "register_layout");

    lua_pushcfunction(L, l_window_apply_layout);
    lua_setfield(L, -2, "apply_layout");

    /* Event hooks */
    lua_pushcfunction(L, l_window_on_create);
    lua_setfield(L, -2, "on_create");

    lua_pushcfunction(L, l_window_on_focus);
    lua_setfield(L, -2, "on_focus");

    lua_pushcfunction(L, l_window_on_close);
    lua_setfield(L, -2, "on_close");

    lua_pushcfunction(L, l_window_on_resize);
    lua_setfield(L, -2, "on_resize");

    /* Session management */
    lua_pushcfunction(L, l_window_serialize);
    lua_setfield(L, -2, "serialize");

    lua_pushcfunction(L, l_window_save_session);
    lua_setfield(L, -2, "save_session");

    lua_pushcfunction(L, l_window_restore_session);
    lua_setfield(L, -2, "restore_session");

    lua_setglobal(L, "window");
}

int lua_bridge_init(Editor *ed) {
    lua_State *L = luaL_newstate();
    if (!L) return -1;

    /* Open standard libraries */
    luaL_openlibs(L);

    /* Store editor pointer in Lua global */
    lua_pushlightuserdata(L, ed);
    lua_setglobal(L, "_EDITOR_PTR");

    /* Register API functions */
    register_buffer_api(L);
    register_editor_api(L);
    register_syntax_api(L);
    register_process_api(L);
    register_terminal_api(L);
    register_window_api(L);

    ed->lua_state = L;
    return 0;
}

void lua_bridge_cleanup(Editor *ed) {
    if (ed && ed->lua_state) {
        lua_close((lua_State *)ed->lua_state);
        ed->lua_state = NULL;
    }
}

int lua_bridge_load_plugin(Editor *ed, const char *filename) {
    if (!ed || !ed->lua_state) return -1;

    lua_State *L = (lua_State *)ed->lua_state;

    if (luaL_dofile(L, filename) != LUA_OK) {
        /* Error occurred - store error message in global _PLUGIN_LOAD_ERROR */
        const char *err = lua_tostring(L, -1);
        if (err) {
            lua_pushstring(L, err);
            lua_setglobal(L, "_PLUGIN_LOAD_ERROR");
        }
        lua_pop(L, 1);
        return -1;
    }

    return 0;
}

int lua_bridge_exec(Editor *ed, const char *code) {
    if (!ed || !ed->lua_state) return -1;

    lua_State *L = (lua_State *)ed->lua_state;

    if (luaL_dostring(L, code) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        (void)err; /* TODO: Display error */
        lua_pop(L, 1);
        return -1;
    }

    return 0;
}

int lua_bridge_call(Editor *ed, const char *func_name) {
    if (!ed || !ed->lua_state) return -1;

    lua_State *L = (lua_State *)ed->lua_state;

    lua_getglobal(L, func_name);
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return -1;
    }

    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        (void)err; /* TODO: Display error */
        lua_pop(L, 1);
        return -1;
    }

    return 0;
}

char *lua_bridge_call_gutter_renderer(Editor *ed, int line_num) {
    if (!ed || !ed->lua_state) return NULL;

    lua_State *L = (lua_State *)ed->lua_state;

    /* Check if _gutter_renderer function exists */
    lua_getglobal(L, "_gutter_renderer");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return NULL;
    }

    /* Call _gutter_renderer(line_num) */
    lua_pushinteger(L, line_num);
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        (void)err; /* TODO: Display error */
        lua_pop(L, 1);
        return NULL;
    }

    /* Get return value */
    const char *result = lua_tostring(L, -1);
    char *gutter = NULL;
    if (result) {
        gutter = strdup(result);
    }
    lua_pop(L, 1);

    return gutter;
}

void lua_bridge_call_window_renderer(Editor *ed, Window *win) {
    if (!ed || !ed->lua_state || !win || !win->renderer_name) return;

    lua_State *L = (lua_State *)ed->lua_state;

    /* Get the renderer table: _window_renderers[renderer_name] */
    lua_getglobal(L, "_window_renderers");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    lua_getfield(L, -1, win->renderer_name);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 2);
        return;
    }

    /* Get the render function from the renderer table */
    lua_getfield(L, -1, "render");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 3);
        return;
    }

    /* Push window data (stored as a Lua registry reference) */
    if (win->content.custom_data) {
        /* Retrieve the data from the registry */
        int ref = (int)(intptr_t)win->content.custom_data;
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    } else {
        lua_newtable(L);  /* Empty table if no data */
    }

    /* Push window dimensions */
    lua_pushinteger(L, win->x);
    lua_pushinteger(L, win->y);
    lua_pushinteger(L, win->width);
    lua_pushinteger(L, win->height);

    /* Call render(data, x, y, width, height) */
    if (lua_pcall(L, 5, 0, 0) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        (void)err; /* TODO: Display error */
        lua_pop(L, 1);
    }

    lua_pop(L, 2);  /* Pop renderer table and _window_renderers */
}

bool lua_bridge_call_window_key_handler(Editor *ed, Window *win, int key) {
    if (!ed || !ed->lua_state || !win || !win->renderer_name) return false;

    lua_State *L = (lua_State *)ed->lua_state;

    /* Get the renderer table */
    lua_getglobal(L, "_window_renderers");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return false;
    }

    lua_getfield(L, -1, win->renderer_name);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 2);
        return false;
    }

    /* Get the on_key function */
    lua_getfield(L, -1, "on_key");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 3);
        return false;
    }

    /* Push window data */
    if (win->content.custom_data) {
        int ref = (int)(intptr_t)win->content.custom_data;
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    } else {
        lua_newtable(L);
    }

    /* Push key */
    lua_pushinteger(L, key);

    /* Call on_key(data, key) -> handled */
    bool handled = false;
    if (lua_pcall(L, 2, 1, 0) == LUA_OK) {
        handled = lua_toboolean(L, -1);
        lua_pop(L, 1);
    } else {
        const char *err = lua_tostring(L, -1);
        (void)err;
        lua_pop(L, 1);
    }

    lua_pop(L, 2);  /* Pop renderer table and _window_renderers */
    return handled;
}

void lua_bridge_trigger_window_event(Editor *ed, const char *event_name, int win_id, int prev_win_id) {
    if (!ed || !ed->lua_state || !event_name) return;

    lua_State *L = (lua_State *)ed->lua_state;

    /* Get event hooks: _window_hooks[event_name] */
    lua_getglobal(L, "_window_hooks");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    lua_getfield(L, -1, event_name);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 2);
        return;
    }

    /* Call all registered callbacks */
    int len = lua_rawlen(L, -1);
    for (int i = 1; i <= len; i++) {
        lua_rawgeti(L, -1, i);
        if (lua_isfunction(L, -1)) {
            /* Push arguments based on event type */
            lua_pushinteger(L, win_id);

            if (strcmp(event_name, "focus") == 0) {
                lua_pushinteger(L, prev_win_id);
                lua_pcall(L, 2, 0, 0);
            } else {
                lua_pcall(L, 1, 0, 0);
            }
        } else {
            lua_pop(L, 1);
        }
    }

    lua_pop(L, 2);  /* Pop hooks table and _window_hooks */
}
