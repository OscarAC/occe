#ifndef LUA_BRIDGE_H
#define LUA_BRIDGE_H

#include "occe.h"

/* Initialize Lua VM and register C functions */
int lua_bridge_init(Editor *ed);

/* Cleanup Lua VM */
void lua_bridge_cleanup(Editor *ed);

/* Load and execute a Lua plugin file */
int lua_bridge_load_plugin(Editor *ed, const char *filename);

/* Execute a Lua string */
int lua_bridge_exec(Editor *ed, const char *code);

/* Call a Lua function */
int lua_bridge_call(Editor *ed, const char *func_name);

/* Call gutter renderer for a specific line */
/* Returns gutter string (must be freed by caller), or NULL if no renderer */
char *lua_bridge_call_gutter_renderer(Editor *ed, int line_num);

/* Call custom window renderer */
/* Renders custom window content at the given position and size */
void lua_bridge_call_window_renderer(Editor *ed, Window *win);

/* Call window key handler for custom windows */
/* Returns true if the key was handled, false otherwise */
bool lua_bridge_call_window_key_handler(Editor *ed, Window *win, int key);

/* Trigger window events */
void lua_bridge_trigger_window_event(Editor *ed, const char *event_name, int win_id, int prev_win_id);

#endif /* LUA_BRIDGE_H */
