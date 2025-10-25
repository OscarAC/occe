# OCCE Plugins

This directory contains Lua plugins for OCCE.

## Available Plugins

### core.lua
Core utility functions for OCCE.

Functions:
- `insert_text(text)` - Insert text at cursor position
- `duplicate_line()` - Duplicate the current line
- `delete_line()` - Delete the current line
- `buffer_info()` - Display buffer information

### hello.lua
Example plugin demonstrating basic functionality.

Functions:
- `say_hello()` - Insert a greeting message
- `insert_timestamp()` - Insert current timestamp
- `insert_separator()` - Insert a separator line

## Creating Plugins

Plugins are Lua scripts that can call OCCE's C API:

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
```

### Editor API
```lua
-- Quit editor
editor.quit()

-- Display message
editor.message("Hello!")
```

## Loading Plugins

Plugins can be loaded:
1. At startup (future feature)
2. Via command mode (future feature)
3. Programmatically from other plugins

Example:
```lua
dofile("plugins/core.lua")
```
