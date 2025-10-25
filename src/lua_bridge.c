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
#include <string.h>

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
    if (!ed || !ed->active_window || !ed->active_window->buffer) {
        return luaL_error(L, "No active buffer");
    }

    int c = luaL_checkinteger(L, 1);
    buffer_insert_char(ed->active_window->buffer, c);
    return 0;
}

/* Lua API: buffer.insert_string(str) */
static int l_buffer_insert_string(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->buffer) {
        return luaL_error(L, "No active buffer");
    }

    size_t len;
    const char *str = luaL_checklstring(L, 1, &len);

    for (size_t i = 0; i < len; i++) {
        buffer_insert_char(ed->active_window->buffer, str[i]);
    }
    return 0;
}

/* Lua API: buffer.delete_char() */
static int l_buffer_delete_char(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->buffer) {
        return luaL_error(L, "No active buffer");
    }

    buffer_delete_char(ed->active_window->buffer);
    return 0;
}

/* Lua API: buffer.insert_newline() */
static int l_buffer_insert_newline(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->buffer) {
        return luaL_error(L, "No active buffer");
    }

    buffer_insert_newline(ed->active_window->buffer);
    return 0;
}

/* Lua API: buffer.save() */
static int l_buffer_save(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->buffer) {
        return luaL_error(L, "No active buffer");
    }

    int result = buffer_save(ed->active_window->buffer);
    lua_pushboolean(L, result == 0);
    return 1;
}

/* Lua API: buffer.get_cursor() -> x, y */
static int l_buffer_get_cursor(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->buffer) {
        return luaL_error(L, "No active buffer");
    }

    Buffer *buf = ed->active_window->buffer;
    lua_pushinteger(L, buf->cursor_x);
    lua_pushinteger(L, buf->cursor_y);
    return 2;
}

/* Lua API: buffer.set_cursor(x, y) */
static int l_buffer_set_cursor(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->buffer) {
        return luaL_error(L, "No active buffer");
    }

    Buffer *buf = ed->active_window->buffer;
    buf->cursor_x = luaL_checkinteger(L, 1);
    buf->cursor_y = luaL_checkinteger(L, 2);
    return 0;
}

/* Lua API: buffer.get_line(y) -> string */
static int l_buffer_get_line(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->buffer) {
        return luaL_error(L, "No active buffer");
    }

    Buffer *buf = ed->active_window->buffer;
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
    if (!ed || !ed->active_window || !ed->active_window->buffer) {
        return luaL_error(L, "No active buffer");
    }

    Buffer *buf = ed->active_window->buffer;
    lua_pushinteger(L, buf->num_rows);
    return 1;
}

/* Lua API: buffer.search(query, forward) -> row, col or nil */
static int l_buffer_search(lua_State *L) {
    Editor *ed = get_editor(L);
    if (!ed || !ed->active_window || !ed->active_window->buffer) {
        return luaL_error(L, "No active buffer");
    }

    Buffer *buf = ed->active_window->buffer;
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
    if (!ed || !ed->active_window || !ed->active_window->buffer) {
        return luaL_error(L, "No active buffer");
    }

    Buffer *buf = ed->active_window->buffer;
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
        /* Error occurred */
        const char *err = lua_tostring(L, -1);
        (void)err; /* TODO: Display error */
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
