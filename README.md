# OCCE

A lightweight, extensible terminal text editor written in C with a powerful Lua-based plugin system. OCCE provides modern editing features while maintaining minimal resource usage and fast startup times.

## Overview

OCCE is designed for developers who want a customizable, keyboard-driven text editor that doesn't sacrifice performance. Built on a clean C core with Lua for extensibility, it offers:

- Fast startup and responsive editing even on large files
- Deep customization through Lua plugins without recompilation
- Advanced window management with splits and tabs
- Syntax highlighting for 14+ languages
- Git integration and visual diff support
- Keyboard-driven editing with customizable keybindings

## Features

### Core Capabilities

- **Multi-buffer editing** - Work with multiple files simultaneously
- **Window splits** - Horizontal and vertical split windows with flexible layouts
- **Tab groups** - Organize windows into named tab collections
- **Undo/Redo** - Full undo stack for each buffer
- **Mouse support** - Click to position cursor, drag to select, scroll wheel support
- **Syntax highlighting** - Built-in support for C, Lua, Python, JavaScript, Rust, Go, Java, Ruby, TypeScript, Shell, HTML, CSS, JSON, Markdown
- **Bracket matching** - Automatic matching of `()`, `{}`, `[]`
- **Search and replace** - Pattern-based search with highlighting
- **Line numbers** - Configurable line number display

### Advanced Features

- **Lua plugin system** - Write plugins in Lua to extend editor functionality
- **Custom keybindings** - Bind any key combination to Lua functions
- **Git integration** - View repository status, branches, and diffs (plugin)
- **Session management** - Save and restore editing sessions (plugin)
- **Word navigation** - Enhanced word-based cursor movement (plugin)
- **Buffer list** - Quick buffer switching interface (plugin)
- **Layout management** - Save and restore window layouts (plugin)
- **Custom renderers** - Create custom window content with Lua
- **Process execution** - Execute shell commands from Lua plugins

## Building from Source

### Dependencies

- C11 compatible compiler (GCC or Clang)
- Make
- Lua 5.4 (can use system Lua or bundled version)
- Standard POSIX libraries

### Optional Dependencies

- `pkg-config` - For automatic Lua detection
- System Lua development files (`lua5.4-dev` on Debian/Ubuntu)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/yourusername/occe.git
cd occe

# Build the editor
make

# Optional: Build bundled Lua if system Lua not found
./build_lua.sh
make

# Run without installing
./occe [filename]
```

### Build Options

```bash
# Debug build with symbols
make debug

# Clean build artifacts
make clean

# Production build with optimizations (default)
make
```

The Makefile automatically detects system Lua via `pkg-config`. If not found, it uses the bundled Lua 5.4.7 (requires running `./build_lua.sh` first).

## Installation

### System-wide Installation

```bash
sudo make install
```

This installs:
- Binary to `/usr/local/bin/occe`
- Default configuration to `~/.config/occe/`
- All plugins to `~/.config/occe/plugins/`

### User-only Installation

```bash
# Copy binary to user bin directory
mkdir -p ~/.local/bin
cp occe ~/.local/bin/

# Set up configuration
mkdir -p ~/.config/occe/plugins
cp -r plugins/* ~/.config/occe/plugins/
cp init.lua.example ~/.config/occe/init.lua
```

Ensure `~/.local/bin` is in your `PATH`.

### Uninstallation

```bash
sudo make uninstall
```

Note: This removes the binary but preserves your configuration directory. Remove `~/.config/occe/` manually if desired.

## Configuration

OCCE is configured through `~/.config/occe/init.lua`, which is executed on startup.

### Basic Configuration Structure

```lua
-- Load syntax highlighting for specific languages
editor.load_plugin("syntax/c.lua")
editor.load_plugin("syntax/lua.lua")
editor.load_plugin("syntax/python.lua")

-- Load core functionality plugins
editor.load_plugin("core.lua")
editor.load_plugin("search.lua")
editor.load_plugin("git.lua")

-- Define custom functions
function my_custom_function()
    buffer.insert_string("Hello, OCCE!")
    editor.message("Custom function executed")
end

-- Bind keys to functions
editor.bind_key(string.byte('h'), editor.KMOD.CTRL, "my_custom_function")
```

### Configuration Locations

OCCE searches for configuration in this order:

1. `./init.lua` - Local directory (development mode)
2. `~/.config/occe/init.lua` - User configuration directory

This allows project-specific configurations and development workflows.

## Usage

### Starting the Editor

```bash
# Open editor with empty buffer
occe

# Open specific file
occe filename.txt

# Open multiple files (opens first, others loaded as buffers)
occe file1.c file2.c file3.c
```

### Basic Keybindings

OCCE uses keyboard shortcuts for efficient editing:

#### Editing

- Regular typing inserts text at cursor
- `Backspace` - Delete character before cursor
- `Delete` - Delete character at cursor
- `Enter` - Insert newline
- Arrow keys - Move cursor
- `Home` / `End` - Jump to start/end of line
- `Page Up` / `Page Down` - Scroll by page

#### File Operations

- `Ctrl+S` - Save current buffer
- `Ctrl+Q` - Quit editor

#### Clipboard

- `Ctrl+C` - Copy selection to clipboard
- `Ctrl+V` - Paste from clipboard

#### Undo/Redo

- `Ctrl+Z` - Undo
- `Ctrl+R` - Redo

#### Window Management

- `Ctrl+W` then `h/j/k/l` - Navigate to window in direction
- `Ctrl+W` then `w` - Cycle to next window
- `Ctrl+W` then `W` - Cycle to previous window

#### Command Mode

Commands can be executed via Lua plugins or the command system:

- `:q` or `:quit` - Quit editor
- `:w` or `:write` - Save buffer
- `:wq` - Save and quit
- `:split [filename]` - Horizontal split
- `:vsplit [filename]` - Vertical split
- `:only` - Close all other windows
- `:ls` or `:buffers` - List all buffers
- `:set number` / `:set nonumber` - Toggle line numbers
- `:lua <code>` - Execute Lua code

### Command-line Options

```bash
occe [filename]          # Open file
occe --version          # Show version information
```

## Plugin System

OCCE's plugin system allows extending functionality without modifying core code.

### Plugin Structure

Plugins are Lua files placed in `~/.config/occe/plugins/`:

```lua
-- Example plugin: my_plugin.lua

-- Define functions
function my_function()
    local x, y = buffer.get_cursor()
    local line = buffer.get_line(y)
    editor.message("Current line: " .. line)
end

-- Register keybindings
editor.bind_key(string.byte('m'), editor.KMOD.CTRL, "my_function")

-- Export functions for use by other plugins
return {
    my_function = my_function
}
```

### Lua API

#### Buffer Operations

```lua
buffer.get_cursor()              -- Returns (x, y)
buffer.set_cursor(x, y)          -- Set cursor position
buffer.get_line(y)               -- Get line content
buffer.get_line_count()          -- Total lines in buffer
buffer.insert_string(text)       -- Insert text at cursor
buffer.insert_newline()          -- Insert newline
buffer.delete_char()             -- Delete character at cursor
buffer.save()                    -- Save current buffer
```

#### Editor Operations

```lua
editor.message(text)             -- Display status message
editor.load_plugin(path)         -- Load plugin file
editor.bind_key(key, mod, func)  -- Bind key to function
editor.quit()                    -- Quit editor
```

#### Process Execution

```lua
process.execute(command)         -- Execute shell command
                                -- Returns: stdout, stderr, exitcode
```

#### Key Modifiers

```lua
editor.KMOD.NONE                 -- No modifier
editor.KMOD.CTRL                 -- Control key
editor.KMOD.ALT                  -- Alt key
editor.KMOD.SHIFT                -- Shift key
```

### Built-in Plugins

OCCE ships with several plugins demonstrating the API:

- **core.lua** - Utility functions (duplicate line, delete line, buffer info)
- **search.lua** - Search and replace functionality
- **word_navigation.lua** - Enhanced word-based movement
- **git.lua** - Git integration (status, diff, branch info)
- **buffer_list.lua** - Interactive buffer switching
- **window_commands.lua** - Advanced window management
- **layouts.lua** - Save/restore window layouts
- **session_manager.lua** - Save/restore editing sessions

### Creating Custom Plugins

1. Create a `.lua` file in `~/.config/occe/plugins/`
2. Define functions and logic
3. Load it in `init.lua`: `editor.load_plugin("your_plugin.lua")`
4. Optionally bind keys to your functions

Example - Simple line numbering toggle:

```lua
-- plugins/toggle_numbers.lua

function toggle_line_numbers()
    -- Toggle would be implemented via editor API
    editor.message("Line numbers toggled")
end

editor.bind_key(string.byte('n'), editor.KMOD.CTRL, "toggle_line_numbers")
```

## Architecture

### Core Components

- **Buffer** (`buffer.c`) - Text content management, undo/redo, syntax highlighting
- **Window** (`window.c`) - Layout management, splits, tabs
- **Editor** (`editor.c`) - Main loop, event handling, state management
- **Terminal** (`terminal.c`) - Raw terminal I/O, escape sequences
- **Lua Bridge** (`lua_bridge.c`) - C ↔ Lua API bindings

### Design Philosophy

1. **C for performance** - Core editing operations in optimized C
2. **Lua for flexibility** - User-facing features as plugins
3. **Separation of concerns** - Clean module boundaries
4. **Minimal dependencies** - Only POSIX + Lua required
5. **Plugin-first development** - New features as plugins when possible

## Development

### Project Structure

```
occe/
├── src/              # C source files
├── include/          # Header files
├── plugins/          # Lua plugins
│   └── syntax/      # Syntax highlighting definitions
├── build/           # Build artifacts
├── Makefile         # Build system
├── init.lua         # Development configuration
└── init.lua.example # Example user configuration
```

### Contributing

Contributions are welcome! Areas of interest:

- New language syntax highlighting
- Additional plugins
- Performance improvements
- Bug fixes
- Documentation improvements

Please ensure code follows the existing style and includes appropriate comments.

## License

MIT License - See LICENSE file for details.

OCCE was built to explore the balance between performance and extensibility in terminal text editors. Designed with a modern plugin architecture from the ground up, combining a performant C core with flexible Lua scripting.
