# OCCE

A high-performance, extensible terminal text editor written in C11 with Lua scripting capabilities. OCCE combines a minimal, efficient core (~6,500 lines of C) with a powerful plugin system for customization without recompilation.

**Version:** 0.1.0
**License:** MIT
**Platform:** POSIX-compliant systems (Linux, BSD, macOS)

## Overview

OCCE is designed for developers who require a fast, keyboard-driven text editor with deep customization capabilities. The architecture prioritizes performance through a native C implementation while providing extensibility through an embedded Lua 5.4 runtime.

### Core Design Principles

1. **Performance-first architecture** - Critical path operations implemented in optimized C
2. **Minimal resource footprint** - Fast startup, low memory usage
3. **Plugin-driven features** - High-level functionality implemented as Lua plugins
4. **Clean separation of concerns** - Modular codebase with well-defined interfaces
5. **Zero mandatory dependencies** - POSIX + bundled Lua (optional system Lua)

### Key Capabilities

- **Multi-buffer editing** with independent undo/redo stacks per buffer
- **Advanced window management** - Horizontal/vertical splits, tab groups, flexible layouts
- **Pluggable renderer architecture** - Terminal (ANSI) and GPU-accelerated (SDL2+OpenGL) backends
- **TrueColor theming system** - 24-bit RGB color support with runtime theme switching
- **Syntax highlighting** - Token-based highlighting for 14+ languages
- **Lua plugin system** - Full API access to editor internals
- **Git integration** - Repository status, diffs, branch information (plugin)
- **Custom keybindings** - Bind any key sequence to Lua functions
- **Visual selection** - Rectangle and line-based selection with clipboard integration

## Architecture

### Component Overview

```
┌─────────────────────────────────────────────────────────┐
│                     Editor (editor.c)                    │
│  Main event loop, state management, command routing      │
└───────────────┬─────────────────────────────────────────┘
                │
        ┌───────┴──────┬──────────────┬──────────────┐
        │              │              │              │
   ┌────▼────┐   ┌────▼────┐   ┌─────▼─────┐   ┌───▼────┐
   │ Buffer  │   │ Window  │   │ Renderer  │   │  Lua   │
   │         │   │         │   │           │   │ Bridge │
   │ buffer.c│   │window.c │   │renderer.c │   │lua_*.c │
   └────┬────┘   └────┬────┘   └─────┬─────┘   └───┬────┘
        │             │              │             │
   ┌────▼────┐   ┌────▼────┐   ┌─────▼─────┐     │
   │  Undo   │   │ TabGrp  │   │  Theme    │     │
   │         │   │         │   │           │     │
   │ undo.c  │   │(embedded)│   │ theme.c  │     │
   └─────────┘   └─────────┘   └───────────┘     │
        │                            │            │
   ┌────▼────┐                  ┌────▼────┐      │
   │ Syntax  │                  │ Colors  │      │
   │         │                  │         │      │
   │syntax.c │                  │colors.c │      │
   └─────────┘                  └─────────┘      │
        │                                         │
   ┌────▼──────────────────────────────┐         │
   │      Terminal (terminal.c)        │◄────────┘
   │  Raw mode, ANSI escape sequences  │
   └───────────────────────────────────┘
```

### Core Modules

#### Buffer (`buffer.c`, `buffer.h`)
- Text storage using gap buffer data structure
- Per-buffer undo/redo stack with operation grouping
- Visual selection state management
- Syntax highlighting cache with multiline state tracking
- Search term highlighting
- Bracket matching algorithm

#### Window (`window.c`, `window.h`)
- Binary space partitioning for split management
- Tab group system for organizing window layouts
- Focus management and navigation
- Custom window types with Lua-defined renderers
- Viewport scrolling and cursor tracking

#### Renderer (`renderer.c`, `renderer.h`)
- Abstract rendering interface
- Terminal backend: ANSI escape sequences, 256-color + TrueColor
- GPU backend: SDL2 + OpenGL (optional compile-time feature)
- Line-based rendering with incremental updates

#### Theme System (`theme.c`, `theme.h`, `colors.c`)
- Runtime theme registration and switching
- TrueColor (24-bit RGB) support with ANSI fallback
- Per-highlight-type color definitions
- UI element theming (statusbar, line numbers, selection)

#### Lua Bridge (`lua_bridge.c`, `lua_bridge.h`)
- Embedded Lua 5.4 interpreter
- C ↔ Lua FFI layer
- Plugin loader with sandboxed execution
- Event system for window/buffer lifecycle hooks
- API exposure: `editor.*`, `buffer.*`, `window.*`, `process.*`

#### Syntax Highlighting (`syntax.c`, `syntax.h`)
- Token-based lexical analysis
- Language definition in Lua
- Multi-line comment state propagation
- Incremental re-highlighting on edits

#### Undo System (`undo.c`, `undo.h`)
- Operation-based undo with delta compression
- Per-buffer undo stacks (configurable depth)
- Undo grouping for logical operations

## Building from Source

### System Requirements

- **Compiler:** GCC ≥ 7.0 or Clang ≥ 6.0 (C11 support required)
- **Build tools:** GNU Make ≥ 4.0
- **Runtime:** POSIX.1-2008 compliant system
- **Lua:** System Lua 5.4 (optional) or bundled Lua 5.4.7

### Dependencies

**Mandatory:**
- Standard C library (glibc, musl, or equivalent)
- `libdl` (dynamic linking)
- `libm` (math library)

**Optional:**
- `pkg-config` - For automatic Lua detection
- Lua 5.4 development files (`lua5.4-dev` on Debian/Ubuntu)

### Build Process

```bash
# Clone repository
git clone https://github.com/yourusername/occe.git
cd occe

# Build with system Lua (if available)
make

# OR build bundled Lua first
./build_lua.sh
make

# Run directly
./occe
```

### Build Targets

```bash
make              # Release build (optimized, stripped)
make debug        # Debug build (-g -O0 -DDEBUG)
make clean        # Remove build artifacts
make install      # System-wide installation (requires root)
make uninstall    # Remove installed binary
```

### Build Configuration

The Makefile automatically detects system Lua via `pkg-config`:

```makefile
# Detected system Lua
LUA_CFLAGS := $(shell pkg-config --cflags lua5.4)
LUA_LDFLAGS := $(shell pkg-config --libs lua5.4)

# Fallback to bundled Lua
LUA_DIR = lua-5.4.7/install
LUA_CFLAGS = -I$(LUA_DIR)/include
LUA_LDFLAGS = -L$(LUA_DIR)/lib -llua
```

**Compiler Flags:**
- `-std=c11` - C11 standard compliance
- `-Wall -Wextra` - All warnings enabled
- `-Os -flto` - Size optimization + Link-Time Optimization (release)
- `-g -O0 -DDEBUG` - Debug symbols + no optimization (debug)

### Bundled Lua Build

If system Lua is unavailable:

```bash
./build_lua.sh
# Compiles Lua 5.4.7 to ./lua-5.4.7/install/
# Static linking to avoid runtime dependencies
```

## Installation

### System-wide Installation

```bash
sudo make install
```

**Installs to:**
- Binary: `/usr/local/bin/occe`
- User config directory: `~/.config/occe/` (created on first run)
- Plugins: Copied to `~/.config/occe/plugins/`

### User-only Installation

```bash
# Install binary to user directory
mkdir -p ~/.local/bin
cp occe ~/.local/bin/

# Configure plugin directory
mkdir -p ~/.config/occe/plugins
cp -r plugins/* ~/.config/occe/plugins/
cp init.lua.example ~/.config/occe/init.lua

# Ensure ~/.local/bin is in PATH
export PATH="$HOME/.local/bin:$PATH"
```

### Uninstallation

```bash
sudo make uninstall
# Note: Preserves ~/.config/occe/ - remove manually if desired
```

## Configuration

OCCE is configured through Lua scripts loaded at startup. Configuration search order:

1. `./init.lua` (working directory - development mode)
2. `~/.config/occe/init.lua` (user configuration)

### Basic Configuration

`~/.config/occe/init.lua`:

```lua
-- Load safe plugin loader (sandboxed execution)
local loader = dofile(editor.config_dir .. "/plugins/plugin_loader.lua")

-- Load syntax highlighting
loader.load_plugins({
    "syntax/c.lua",
    "syntax/lua.lua",
    "syntax/python.lua",
    "syntax/rust.lua"
})

-- Load core functionality
loader.load_plugins({
    "core.lua",           -- Utility functions
    "search.lua",         -- Search/replace
    "git.lua",            -- Git integration
    "buffer_list.lua"     -- Buffer management
})

-- Custom keybindings
function quick_save()
    buffer.save()
    editor.message("Saved: " .. buffer.get_filename())
end

editor.bind_key(string.byte('s'), editor.KMOD.CTRL, "quick_save")

-- Theme configuration
editor.set_theme("monokai")
```

### Configuration API

#### Editor Settings

```lua
editor.message(text)                     -- Display statusbar message
editor.quit()                            -- Exit editor
editor.load_plugin(path)                 -- Load Lua plugin
editor.bind_key(key, modifiers, func)    -- Bind key to function name
editor.set_theme(name)                   -- Switch theme
editor.config_dir                        -- Path to ~/.config/occe/
```

#### Key Modifiers

```lua
editor.KMOD.NONE    -- No modifier
editor.KMOD.CTRL    -- Control key
editor.KMOD.ALT     -- Alt key
editor.KMOD.SHIFT   -- Shift key
```

#### Key Codes

Use `string.byte('x')` for ASCII keys, or SDL keycodes for special keys.

## Usage

### Command-line Interface

```bash
occe                    # Open with empty buffer
occe file.txt           # Open specific file
occe file1 file2 file3  # Open multiple files (multi-buffer)
occe --version          # Display version information
```

### Keybindings

#### Navigation
- `Arrow keys` - Move cursor
- `Home` / `End` - Line start/end
- `Ctrl+Home` / `Ctrl+End` - Buffer start/end
- `Page Up` / `Page Down` - Scroll by screen

#### Editing
- `Backspace` - Delete character before cursor
- `Delete` - Delete character at cursor
- `Enter` - Insert newline
- `Tab` - Insert tab/spaces

#### File Operations
- `Ctrl+S` - Save current buffer
- `Ctrl+Q` - Quit editor (prompts if unsaved)

#### Clipboard
- `Ctrl+C` - Copy selection
- `Ctrl+V` - Paste
- `Ctrl+X` - Cut selection

#### Undo/Redo
- `Ctrl+Z` - Undo last operation
- `Ctrl+R` - Redo undone operation

#### Window Management
- `Ctrl+W` then `h/j/k/l` - Navigate to window (left/down/up/right)
- `Ctrl+W` then `w` - Cycle to next window
- `Ctrl+W` then `W` - Cycle to previous window
- `Ctrl+W` then `s` - Horizontal split
- `Ctrl+W` then `v` - Vertical split
- `Ctrl+W` then `o` - Close all other windows

#### Command Mode
- `:` - Enter command mode (if configured)
- `:q` / `:quit` - Quit
- `:w` / `:write` - Save
- `:wq` - Save and quit
- `:split [file]` - Horizontal split
- `:vsplit [file]` - Vertical split
- `:only` - Close other windows
- `:buffers` / `:ls` - List buffers
- `:lua <code>` - Execute Lua code

## Plugin System

### Plugin Architecture

Plugins are Lua scripts with access to the editor API. The plugin system supports:

- **Function registration** - Export functions for keybindings/commands
- **Event hooks** - React to editor events (buffer open, window focus, etc.)
- **Custom windows** - Define custom UI elements
- **Gutter renderers** - Custom line number/gutter content
- **Process execution** - Shell command integration

### Plugin Structure

`~/.config/occe/plugins/example.lua`:

```lua
-- Plugin metadata (optional)
local plugin = {
    name = "Example Plugin",
    version = "1.0.0",
    author = "Your Name"
}

-- Define functions
function my_custom_function()
    local x, y = buffer.get_cursor()
    local line = buffer.get_line(y)
    editor.message("Line " .. y .. ": " .. line)
end

-- Register keybindings
editor.bind_key(string.byte('e'), editor.KMOD.CTRL, "my_custom_function")

-- Export API (optional)
return plugin
```

### Complete API Reference

#### Buffer Operations

```lua
-- Cursor manipulation
buffer.get_cursor()                     -- Returns (x, y)
buffer.set_cursor(x, y)                 -- Set cursor position

-- Line operations
buffer.get_line(y)                      -- Get line content (0-indexed)
buffer.get_line_count()                 -- Total lines in buffer
buffer.insert_line(y, text)             -- Insert new line at position
buffer.delete_line(y)                   -- Delete line

-- Content manipulation
buffer.insert_string(text)              -- Insert at cursor
buffer.insert_char(c)                   -- Insert single character
buffer.insert_newline()                 -- Insert newline at cursor
buffer.delete_char()                    -- Delete char at cursor
buffer.delete_selection()               -- Delete selected text

-- File operations
buffer.save()                           -- Save to disk
buffer.get_filename()                   -- Get current filename
buffer.is_modified()                    -- Check if modified

-- Selection
buffer.start_selection()                -- Begin visual selection
buffer.get_selection()                  -- Get selected text
buffer.has_selection()                  -- Check if selection active

-- Search
buffer.set_search_term(pattern)         -- Highlight search matches
buffer.clear_search()                   -- Clear search highlighting
```

#### Window Operations

```lua
-- Window management
window.split_horizontal()               -- Split current window horizontally
window.split_vertical()                 -- Split current window vertically
window.close()                          -- Close current window
window.focus_next()                     -- Focus next window
window.focus_prev()                     -- Focus previous window

-- Custom windows
window.create_custom(name, renderer_func, key_handler_func)
-- Creates a custom window with Lua-defined content

-- Window properties
window.get_width()                      -- Get window width
window.get_height()                     -- Get window height
window.get_id()                         -- Get unique window ID
```

#### Editor Operations

```lua
-- Core functions
editor.message(text)                    -- Display status message
editor.error(text)                      -- Display error message
editor.quit()                           -- Quit editor
editor.load_plugin(path)                -- Load plugin file

-- Keybindings
editor.bind_key(key, modifiers, function_name)
-- Bind key combination to Lua function (by name)

-- Theme management
editor.set_theme(name)                  -- Switch to named theme
editor.get_theme()                      -- Get current theme name
editor.list_themes()                    -- List available themes

-- Buffer management
editor.open_buffer(filename)            -- Open file in new buffer
editor.close_buffer(index)              -- Close buffer by index
editor.list_buffers()                   -- Get list of open buffers
editor.switch_buffer(index)             -- Switch to buffer by index

-- Properties
editor.config_dir                       -- Path to config directory
```

#### Process Execution

```lua
process.execute(command)
-- Execute shell command
-- Returns: stdout (string), stderr (string), exit_code (integer)

-- Example:
local stdout, stderr, exit_code = process.execute("git status")
if exit_code == 0 then
    editor.message("Git status: " .. stdout)
end
```

#### Theme API

```lua
-- Define themes in Lua
theme.create({
    name = "My Theme",
    author = "Your Name",
    dark_mode = true,

    -- UI colors (TrueColor RGB)
    background = {r = 30, g = 30, b = 30},
    foreground = {r = 220, g = 220, b = 220},
    selection_bg = {r = 80, g = 80, b = 120},
    line_number = {r = 100, g = 100, b = 100},

    -- Syntax highlighting colors
    colors = {
        keyword = {fg = {r = 249, g = 38, b = 114}},
        string = {fg = {r = 230, g = 219, b = 116}},
        comment = {fg = {r = 117, g = 113, b = 94}},
        -- ... more highlight types
    }
})

-- Activate theme
editor.set_theme("My Theme")
```

### Built-in Plugins

OCCE ships with reference plugins:

| Plugin | Description | Functions |
|--------|-------------|-----------|
| `core.lua` | Utility functions | `duplicate_line()`, `delete_line()`, `buffer_info()` |
| `search.lua` | Search/replace | `search_forward()`, `search_backward()`, `replace_all()` |
| `word_navigation.lua` | Enhanced navigation | `word_forward()`, `word_backward()`, `word_delete()` |
| `git.lua` | Git integration | `git_status()`, `git_diff()`, `git_branch()` |
| `buffer_list.lua` | Buffer switcher | `show_buffer_list()` (interactive menu) |
| `window_commands.lua` | Window management | Various split/focus commands |
| `layouts.lua` | Layout persistence | `save_layout()`, `load_layout()` |
| `session_manager.lua` | Session restore | `save_session()`, `restore_session()` |
| `themes.lua` | Theme definitions | Predefined color schemes |

### Creating Custom Plugins

**Example: Line counter plugin**

`~/.config/occe/plugins/line_counter.lua`:

```lua
function count_lines_in_selection()
    if not buffer.has_selection() then
        editor.message("No selection")
        return
    end

    local selection = buffer.get_selection()
    local line_count = 0
    for _ in selection:gmatch("\n") do
        line_count = line_count + 1
    end
    line_count = line_count + 1  -- Count last line

    editor.message("Selection: " .. line_count .. " lines")
end

-- Bind to Ctrl+L
editor.bind_key(string.byte('l'), editor.KMOD.CTRL, "count_lines_in_selection")
```

Load in `init.lua`:

```lua
editor.load_plugin("line_counter.lua")
```

## Performance Characteristics

### Startup Time
- **Empty buffer:** < 5ms (typical)
- **With plugins:** < 20ms (10+ plugins)
- **Large file (10MB):** < 100ms

### Memory Usage
- **Base editor:** ~2MB resident
- **Per buffer:** ~size of file + 10-20% (undo stack)
- **Lua runtime:** ~1MB

### Rendering Performance
- **Terminal backend:** 60+ FPS on typical terminals
- **GPU backend:** 144+ FPS (with compatible hardware)

### File Size Limits
- **Practical limit:** Tested up to 500MB files
- **Line limit:** 2^31 lines (theoretical)
- **Line length:** 2^31 characters per line (theoretical)

## Development

### Project Structure

```
occe/
├── src/                    # C source files
│   ├── main.c             # Entry point
│   ├── editor.c           # Core editor logic
│   ├── buffer.c           # Text buffer management
│   ├── window.c           # Window/split management
│   ├── renderer.c         # Rendering abstraction
│   ├── terminal.c         # Terminal I/O
│   ├── syntax.c           # Syntax highlighting
│   ├── theme.c            # Theme system
│   ├── lua_bridge.c       # Lua integration
│   └── ...
├── include/                # Header files
│   ├── occe.h             # Main header
│   ├── buffer.h           # Buffer interface
│   └── ...
├── plugins/                # Lua plugins
│   ├── core.lua
│   ├── search.lua
│   ├── git.lua
│   ├── syntax/            # Language definitions
│   │   ├── c.lua
│   │   ├── lua.lua
│   │   └── ...
│   └── plugin_loader.lua  # Safe plugin loader
├── build/                  # Build artifacts (generated)
├── lua-5.4.7/             # Bundled Lua (optional)
├── Makefile               # Build system
├── build_lua.sh           # Lua build script
├── init.lua               # Development config
├── init.lua.example       # Example user config
├── LICENSE                # MIT License
└── README.md              # This file
```

### Coding Standards

- **C11 standard** with POSIX extensions
- **Naming:** `snake_case` for functions/variables, `PascalCase` for types
- **Header guards:** `#ifndef MODULENAME_H`
- **Error handling:** Return `-1` or `NULL` on error, set `errno` where applicable
- **Memory management:** Explicit ownership, free paired with alloc
- **Comments:** Doxygen-style for public APIs

### Adding Features

1. **Pure C features:** Modify core modules (`src/`, `include/`)
2. **Extensible features:** Implement as Lua plugins
3. **Language support:** Add syntax definitions in `plugins/syntax/`

### Debugging

```bash
# Build with debug symbols
make debug

# Run under GDB
gdb ./occe

# Run under Valgrind
valgrind --leak-check=full ./occe
```

### Testing

Currently manual testing. Contributions for automated tests welcome.

## Contributing

Contributions are welcome! Areas of interest:

### High Priority
- [ ] Comprehensive test suite (unit + integration)
- [ ] Additional language syntax definitions
- [ ] Performance profiling and optimization
- [ ] Documentation improvements
- [ ] Bug fixes

### Feature Requests
- [ ] LSP (Language Server Protocol) integration
- [ ] Fuzzy file finder
- [ ] Multiple cursor support
- [ ] Configurable mouse actions
- [ ] Terminal multiplexer integration
- [ ] Remote editing over SSH

### Contribution Guidelines

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Commit** changes with clear messages
4. **Ensure** code follows existing style
5. **Test** thoroughly (manual until test suite exists)
6. **Submit** a pull request

**Code Review Criteria:**
- Clean, readable code
- Minimal performance impact
- No memory leaks (run Valgrind)
- Follows project architecture

## License

MIT License - See [LICENSE](LICENSE) file for details.

## Acknowledgments

OCCE draws inspiration from:
- **Vim/Neovim** - Modal editing paradigm
- **Kakoune** - Multiple selection concepts
- **kilo** - Minimalist terminal editor architecture
- **vis** - Structural regular expressions

Built with:
- **Lua 5.4** - Embedded scripting engine
- **POSIX APIs** - Cross-platform compatibility

---

**Project Status:** Active development (v0.1.0)
