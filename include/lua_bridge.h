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

#endif /* LUA_BRIDGE_H */
