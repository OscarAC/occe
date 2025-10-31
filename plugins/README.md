# OCCE Plugins

This directory contains plugins that extend OCCE's functionality. All plugins are written in Lua.

## Directory Structure

```
plugins/
├── core/               # Essential functionality
│   ├── core.lua       # Core key bindings (Ctrl+S, Ctrl+Z, etc.)
│   └── plugin_loader.lua  # Safe plugin loading system
├── features/          # Optional features
│   ├── buffer_list.lua    # Buffer management
│   ├── git.lua           # Git integration
│   ├── keybindings.lua   # Additional key bindings
│   ├── layouts.lua       # Window layout management
│   ├── search.lua        # Search functionality
│   ├── session_manager.lua # Session persistence
│   ├── shift_selection.lua # Shift+Arrow text selection
│   ├── themes.lua        # Theme management
│   ├── window_commands.lua # Window manipulation
│   └── word_navigation.lua # Word-based navigation
└── syntax/            # Syntax highlighting
    ├── c.lua
    ├── lua.lua
    ├── python.lua
    └── ... (14 languages total)
```

## Core Plugins

These are loaded by default and provide essential functionality:

- **core.lua**: Defines all basic key bindings (save, quit, undo, redo, copy, paste, tab navigation)
- **plugin_loader.lua**: Provides safe plugin loading with error handling

## Feature Plugins

Optional plugins that can be enabled/disabled in `init.lua`:

- **search.lua**: Buffer search functionality
- **word_navigation.lua**: Word-based cursor movement
- **shift_selection.lua**: Text selection using Shift+Arrow keys
- **git.lua**: Git status integration
- **window_commands.lua**: Advanced window management
- **buffer_list.lua**: Buffer list and switching
- **layouts.lua**: Predefined window layouts
- **session_manager.lua**: Save and restore editing sessions

## Syntax Plugins

Syntax highlighting for various programming languages. Each file defines keywords, operators, and highlighting rules for a specific language.

## Creating Custom Plugins

### Editor API

```lua
-- File operations
editor.save()              -- Save current buffer
editor.open(filename)      -- Open a file
editor.quit()              -- Quit editor

-- Editing
editor.undo()              -- Undo last change
editor.redo()              -- Redo last undone change
editor.copy()              -- Copy selection
editor.paste()             -- Paste from clipboard

-- Tabs
editor.tabnew([filename])  -- Create new tab
editor.tabnext()           -- Next tab
editor.tabprev()           -- Previous tab
editor.tabclose()          -- Close current tab

-- Windows/Splits
editor.split([filename])   -- Horizontal split
editor.vsplit([filename])  -- Vertical split
editor.window_next()       -- Next window
editor.window_prev()       -- Previous window

-- Settings
editor.set_tab_width(n)    -- Set tab width
editor.set_use_spaces(bool)-- Use spaces instead of tabs
editor.message(str)        -- Display status message

-- Key bindings
editor.bind_key(key, modifiers, function_name)
editor.unbind_key(key, modifiers)
```

### Buffer API

```lua
-- Insert operations
buffer.insert_char(char_code)
buffer.insert_string(string)
buffer.insert_newline()

-- Delete operations
buffer.delete_char()

-- Cursor operations
local x, y = buffer.get_cursor()
buffer.set_cursor(x, y)

-- Line operations
local line = buffer.get_line(line_number)
local count = buffer.get_line_count()

-- Save buffer
local success = buffer.save()

-- Search and replace
local result = buffer.search(query)
local count = buffer.replace(search, replace, all)

-- Selection operations
buffer.start_selection()        -- Start selection at current cursor
buffer.clear_selection()        -- Clear current selection
local has_sel = buffer.has_selection()  -- Check if selection exists
```

### Key Constants

```lua
editor.KEY.CTRL_C, CTRL_D, CTRL_Q, CTRL_R, CTRL_S, CTRL_V, CTRL_W, CTRL_Z
editor.KEY.CTRL_ARROW_LEFT, CTRL_ARROW_RIGHT, CTRL_ARROW_UP, CTRL_ARROW_DOWN
editor.KEY.SHIFT_ARROW_LEFT, SHIFT_ARROW_RIGHT, SHIFT_ARROW_UP, SHIFT_ARROW_DOWN
editor.KEY.CTRL_PAGE_UP, CTRL_PAGE_DOWN
editor.KEY.ARROW_LEFT, ARROW_RIGHT, ARROW_UP, ARROW_DOWN

editor.KMOD.NONE, CTRL, ALT, SHIFT
```

### Example Plugin

```lua
-- my_plugin.lua
function save_and_quit()
    editor.save()
    editor.quit()
end

-- Bind to Ctrl+X
editor.bind_key(string.byte('x'), editor.KMOD.CTRL, "save_and_quit")

editor.message("My Plugin Loaded")
```

## Loading Plugins

Add your plugin to `init.lua`:

```lua
local loader = dofile("./plugins/core/plugin_loader.lua")

loader.load_plugins({
    "features/my_plugin.lua"
})
```

The plugin loader provides automatic error handling and will report any issues without crashing the editor.
