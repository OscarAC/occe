#ifndef KEYBIND_H
#define KEYBIND_H

#include "occe.h"

/* Key modifiers */
#define KMOD_NONE  0
#define KMOD_CTRL  (1 << 0)
#define KMOD_ALT   (1 << 1)
#define KMOD_SHIFT (1 << 2)

/* Keybinding entry */
typedef struct KeyBinding {
    int key;
    int modifiers;
    char *lua_function;
    struct KeyBinding *next;
} KeyBinding;

/* Keybinding map */
typedef struct KeyMap {
    KeyBinding *bindings;
} KeyMap;

/* Initialize keybinding system */
KeyMap *keymap_create(void);
void keymap_destroy(KeyMap *map);

/* Add/remove keybindings */
void keymap_bind(KeyMap *map, int key, int modifiers, const char *lua_func);
void keymap_unbind(KeyMap *map, int key, int modifiers);

/* Execute keybinding */
int keymap_execute(KeyMap *map, Editor *ed, int key, int modifiers);

/* Helper to check if key has Ctrl modifier */
static inline int is_ctrl_key(int key) {
    return key >= 1 && key <= 26;
}

#endif /* KEYBIND_H */
