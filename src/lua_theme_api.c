#define _POSIX_C_SOURCE 200809L
#include "lua_bridge.h"
#include "theme.h"
#include "colors.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>

/* Lua API: theme.list() -> array of theme names */
static int l_theme_list(lua_State *L) {
    lua_newtable(L);
    int index = 1;

    for (Theme *t = theme_list; t; t = t->next) {
        lua_pushstring(L, t->name);
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

/* Lua API: theme.get_active() -> theme name */
static int l_theme_get_active(lua_State *L) {
    Theme *theme = theme_get_active();
    if (theme) {
        lua_pushstring(L, theme->name);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

/* Lua API: theme.set_active(name) -> boolean */
static int l_theme_set_active(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    Theme *theme = theme_find(name);

    if (theme) {
        theme_set_active(theme);
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);
    }

    return 1;
}

/* Lua API: theme.create(name) -> theme object */
static int l_theme_create(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    Theme *theme = theme_create(name);

    if (!theme) {
        lua_pushnil(L);
        return 1;
    }

    /* Return theme as a table */
    lua_newtable(L);
    lua_pushstring(L, theme->name);
    lua_setfield(L, -2, "name");

    /* Store pointer to theme */
    lua_pushlightuserdata(L, theme);
    lua_setfield(L, -2, "_ptr");

    return 1;
}

/* Lua API: theme.register(theme_object) */
static int l_theme_register(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "_ptr");
    Theme *theme = (Theme *)lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (theme) {
        theme_register(theme);
    }

    return 0;
}

/* Helper: Parse color from Lua table or string */
static ThemeColor parse_color(lua_State *L, int index) {
    if (lua_type(L, index) == LUA_TSTRING) {
        const char *str = lua_tostring(L, index);
        if (str[0] == '#') {
            /* Hex color */
            return theme_hex(str);
        } else {
            /* ANSI color name */
            if (strcmp(str, "black") == 0) return theme_ansi(COLOR_BLACK);
            if (strcmp(str, "red") == 0) return theme_ansi(COLOR_RED);
            if (strcmp(str, "green") == 0) return theme_ansi(COLOR_GREEN);
            if (strcmp(str, "yellow") == 0) return theme_ansi(COLOR_YELLOW);
            if (strcmp(str, "blue") == 0) return theme_ansi(COLOR_BLUE);
            if (strcmp(str, "magenta") == 0) return theme_ansi(COLOR_MAGENTA);
            if (strcmp(str, "cyan") == 0) return theme_ansi(COLOR_CYAN);
            if (strcmp(str, "white") == 0) return theme_ansi(COLOR_WHITE);
            if (strcmp(str, "bright_black") == 0) return theme_ansi(COLOR_BRIGHT_BLACK);
            if (strcmp(str, "bright_red") == 0) return theme_ansi(COLOR_BRIGHT_RED);
            if (strcmp(str, "bright_green") == 0) return theme_ansi(COLOR_BRIGHT_GREEN);
            if (strcmp(str, "bright_yellow") == 0) return theme_ansi(COLOR_BRIGHT_YELLOW);
            if (strcmp(str, "bright_blue") == 0) return theme_ansi(COLOR_BRIGHT_BLUE);
            if (strcmp(str, "bright_magenta") == 0) return theme_ansi(COLOR_BRIGHT_MAGENTA);
            if (strcmp(str, "bright_cyan") == 0) return theme_ansi(COLOR_BRIGHT_CYAN);
            if (strcmp(str, "bright_white") == 0) return theme_ansi(COLOR_BRIGHT_WHITE);
            return theme_ansi(COLOR_DEFAULT);
        }
    } else if (lua_type(L, index) == LUA_TTABLE) {
        /* RGB table: {r=255, g=0, b=0} */
        lua_getfield(L, index, "r");
        lua_getfield(L, index, "g");
        lua_getfield(L, index, "b");

        uint8_t r = lua_tointeger(L, -3);
        uint8_t g = lua_tointeger(L, -2);
        uint8_t b = lua_tointeger(L, -1);

        lua_pop(L, 3);
        return theme_rgb(r, g, b);
    }

    return theme_ansi(COLOR_DEFAULT);
}

/* Lua API: theme.set_color(theme_obj, hl_type, fg, bg, attrs) */
static int l_theme_set_color(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "_ptr");
    Theme *theme = (Theme *)lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (!theme) {
        return luaL_error(L, "Invalid theme object");
    }

    /* Get highlight type */
    HighlightType hl_type = luaL_checkinteger(L, 2);
    if (hl_type < 0 || hl_type >= HL_MAX) {
        return luaL_error(L, "Invalid highlight type");
    }

    /* Parse colors */
    ThemeColor fg = parse_color(L, 3);
    ThemeColor bg = parse_color(L, 4);

    /* Parse attributes (optional) */
    uint8_t attrs = 0;
    if (lua_istable(L, 5)) {
        lua_getfield(L, 5, "bold");
        if (lua_toboolean(L, -1)) attrs |= (1 << ATTR_BOLD);
        lua_pop(L, 1);

        lua_getfield(L, 5, "italic");
        if (lua_toboolean(L, -1)) attrs |= (1 << ATTR_ITALIC);
        lua_pop(L, 1);

        lua_getfield(L, 5, "underline");
        if (lua_toboolean(L, -1)) attrs |= (1 << ATTR_UNDERLINE);
        lua_pop(L, 1);

        lua_getfield(L, 5, "reverse");
        if (lua_toboolean(L, -1)) attrs |= (1 << ATTR_REVERSE);
        lua_pop(L, 1);
    }

    theme_set_color(theme, hl_type, fg, bg, attrs);
    return 0;
}

/* Lua API: theme.get_info(name) -> table with theme info */
static int l_theme_get_info(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    Theme *theme = theme_find(name);

    if (!theme) {
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);

    lua_pushstring(L, theme->name);
    lua_setfield(L, -2, "name");

    if (theme->author) {
        lua_pushstring(L, theme->author);
        lua_setfield(L, -2, "author");
    }

    if (theme->description) {
        lua_pushstring(L, theme->description);
        lua_setfield(L, -2, "description");
    }

    lua_pushboolean(L, theme->dark_mode);
    lua_setfield(L, -2, "dark_mode");

    return 1;
}

/* Lua API: theme.rgb(r, g, b) -> color object */
static int l_theme_rgb(lua_State *L) {
    uint8_t r = luaL_checkinteger(L, 1);
    uint8_t g = luaL_checkinteger(L, 2);
    uint8_t b = luaL_checkinteger(L, 3);

    lua_newtable(L);
    lua_pushinteger(L, r);
    lua_setfield(L, -2, "r");
    lua_pushinteger(L, g);
    lua_setfield(L, -2, "g");
    lua_pushinteger(L, b);
    lua_setfield(L, -2, "b");

    return 1;
}

/* Lua API: theme.hex(hex_string) -> color object */
static int l_theme_hex(lua_State *L) {
    const char *hex = luaL_checkstring(L, 1);
    ThemeColor tc = theme_hex(hex);

    if (tc.is_rgb) {
        lua_newtable(L);
        lua_pushinteger(L, tc.rgb.r);
        lua_setfield(L, -2, "r");
        lua_pushinteger(L, tc.rgb.g);
        lua_setfield(L, -2, "g");
        lua_pushinteger(L, tc.rgb.b);
        lua_setfield(L, -2, "b");
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

/* Register theme API functions */
void register_theme_api(lua_State *L) {
    lua_newtable(L);

    /* Functions */
    lua_pushcfunction(L, l_theme_list);
    lua_setfield(L, -2, "list");

    lua_pushcfunction(L, l_theme_get_active);
    lua_setfield(L, -2, "get_active");

    lua_pushcfunction(L, l_theme_set_active);
    lua_setfield(L, -2, "set_active");

    lua_pushcfunction(L, l_theme_create);
    lua_setfield(L, -2, "create");

    lua_pushcfunction(L, l_theme_register);
    lua_setfield(L, -2, "register");

    lua_pushcfunction(L, l_theme_set_color);
    lua_setfield(L, -2, "set_color");

    lua_pushcfunction(L, l_theme_get_info);
    lua_setfield(L, -2, "get_info");

    lua_pushcfunction(L, l_theme_rgb);
    lua_setfield(L, -2, "rgb");

    lua_pushcfunction(L, l_theme_hex);
    lua_setfield(L, -2, "hex");

    /* Highlight type constants (from colors.h) */
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
    lua_pushinteger(L, HL_ERROR);
    lua_setfield(L, -2, "HL_ERROR");

    lua_setglobal(L, "theme");
}
