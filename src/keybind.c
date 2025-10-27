#define _POSIX_C_SOURCE 200809L
#include "keybind.h"
#include "lua_bridge.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

KeyMap *keymap_create(void) {
    KeyMap *map = malloc(sizeof(KeyMap));
    if (!map) return NULL;

    map->bindings = NULL;
    return map;
}

void keymap_destroy(KeyMap *map) {
    if (!map) return;

    KeyBinding *kb = map->bindings;
    while (kb) {
        KeyBinding *next = kb->next;
        if (kb->lua_function) free(kb->lua_function);
        free(kb);
        kb = next;
    }

    free(map);
}

void keymap_bind(KeyMap *map, int key, int modifiers, const char *lua_func) {
    if (!map || !lua_func) return;

    /* Check if binding already exists */
    KeyBinding *kb = map->bindings;
    while (kb) {
        if (kb->key == key && kb->modifiers == modifiers) {
            /* Update existing binding */
            if (kb->lua_function) free(kb->lua_function);
            kb->lua_function = strdup(lua_func);
            return;
        }
        kb = kb->next;
    }

    /* Create new binding */
    kb = malloc(sizeof(KeyBinding));
    if (!kb) return;

    kb->key = key;
    kb->modifiers = modifiers;
    kb->lua_function = strdup(lua_func);
    kb->next = map->bindings;
    map->bindings = kb;
}

void keymap_unbind(KeyMap *map, int key, int modifiers) {
    if (!map) return;

    KeyBinding **kb_ptr = &map->bindings;
    while (*kb_ptr) {
        KeyBinding *kb = *kb_ptr;
        if (kb->key == key && kb->modifiers == modifiers) {
            *kb_ptr = kb->next;
            if (kb->lua_function) free(kb->lua_function);
            free(kb);
            return;
        }
        kb_ptr = &kb->next;
    }
}

int keymap_execute(KeyMap *map, Editor *ed, int key, int modifiers) {
    if (!map || !ed) return -1;

    KeyBinding *kb = map->bindings;
    while (kb) {
        if (kb->key == key && kb->modifiers == modifiers) {
            /* Execute the Lua function */
            if (lua_bridge_call(ed, kb->lua_function) == 0) {
                return 0; /* Success */
            }
            return -1; /* Execution failed */
        }
        kb = kb->next;
    }

    return -1; /* No binding found */
}
