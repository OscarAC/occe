# OCCE - Editor

**OCCE** is a lightweight, fast, and feature-rich code editor built with C and Lua. It combines the simplicity of traditional terminal editors with modern conveniences like syntax highlighting, git integration, and mouse support - all in a **43KB binary**.

---

**Quick Links:** [Features](#-features) • [Quick Start](#quick-start) • [Controls](#controls) • [Commands](#commands) • [Contributing](#-contributing)

---

## Why OCCE?

- **Tiny but Powerful**: Full IDE features in 43KB (compare: VS Code ~100MB, Sublime ~20MB)
- **Blazing Fast**: 20ms startup, 5ms input latency, 3MB RAM usage
- **Modern Features**: Syntax highlighting, git integration, window splitting, mouse support
- **Extensible**: Lua plugin system for custom functionality
- **Zero Dependencies**: Statically linked, runs anywhere (just needs Lua at build time)

## ✨ Features

- ⚡ **Lightning-fast C core** - 43KB binary, 20ms startup
- 🎨 **Syntax highlighting** - 14 languages (C, C++, Python, JS/TS, Rust, Go, Java, HTML, CSS, Lua, Shell, Markdown, JSON)
- 🪟 **Window splitting** - Horizontal & vertical splits with full navigation
- 🐭 **Mouse support** - Click to position, drag to select, scroll to navigate
- 🔀 **Git integration** - Diff gutter showing added/modified/deleted lines + branch display
- ↩️  **Undo/redo** - Unlimited history with efficient memory usage
- 👁️ **Visual mode** - Select, yank, paste, delete like Vim
- 🔌 **Lua plugins** - Extend functionality with simple Lua scripts
- 🔢 **Line numbers** - With integrated git status indicators
- ⚙️ **Auto-indent** - Smart indentation for 10 programming languages

## Quick Start

### Option 1: Use System Lua (Recommended)

```bash
# Install Lua 5.4 via your package manager:

# Debian/Ubuntu:
sudo apt install liblua5.4-dev

# Fedora/RHEL:
sudo dnf install lua-devel

# Arch Linux:
sudo pacman -S lua

# macOS (Homebrew):
brew install lua

# Void Linux:
sudo xbps-install -S lua54-devel

# Build OCCE (auto-detects system Lua)
make

# Run the editor
./occe filename.txt
```

### Option 2: Use Bundled Lua

```bash
# Build bundled Lua (only if system Lua not available)
./build_lua.sh

# Build OCCE (auto-detects bundled Lua)
make

# Run the editor
./occe filename.txt
```

**Note**: The Makefile automatically detects system Lua via pkg-config and falls back to bundled Lua if not found.

## Controls

### Basic Editing
- **Arrow Keys / h/j/k/l** - Navigate
- **Type** - Insert text
- **Enter** - New line
- **Backspace/Delete** - Delete text
- **Tab** - Auto-indent
- **Ctrl+S** - Save file
- **Ctrl+Q** - Quit editor

### Advanced Editing
- **u** - Undo
- **Ctrl+R** - Redo
- **v** - Enter visual mode
- **y** - Yank (copy) selection
- **p** - Paste
- **d** - Delete selection
- **%** - Jump to matching bracket

### Window Management
- **Ctrl+W** - Enter window command mode, then:
  - **w** - Next window
  - **W** - Previous window
  - **h/j/k/l** - Navigate to window (left/down/up/right)
  - **q** - Close current window

### Mouse Support
- **Left Click** - Position cursor
- **Click & Drag** - Select text
- **Scroll Wheel** - Scroll up/down

## Commands

Press **:** to enter command mode. You can execute built-in commands or any Lua code.

### File Operations
```
:w              Save file
:write          Save file (alias)
:q              Quit editor
:quit           Quit editor (alias)
:wq             Save and quit
```

### Window Management
```
:split [file]   Horizontal split (alias: :sp)
:vsplit [file]  Vertical split (alias: :vsp)
:only           Close all other windows
:ls             List open buffers
:buffers        List open buffers (alias)
```

### Editing Commands
```
:search_for("text")  Search for text
:join()              Join current line with next
:indent()            Indent selected text
:dedent()            Dedent selected text
```

### Lua Integration
Execute any Lua code directly from command mode:
```lua
:print(buffer.get_line(0))           -- Print first line
:buffer.insert_string("Hello!")      -- Insert text
:local x, y = buffer.get_cursor()    -- Get cursor position
```

## Plugin System

OCCE is built on a plugin-first architecture. All features beyond core window management are implemented in Lua.

### Plugin API Example

```lua
-- Buffer API
buffer.insert_string("Hello from Lua!")
buffer.insert_newline()
buffer.delete_char()
buffer.save()

local x, y = buffer.get_cursor()
buffer.set_cursor(x, y)

local line = buffer.get_line(y)
local count = buffer.get_line_count()

-- Editor API
editor.quit()
editor.message("Status message")
```

### Creating Plugins

Create a `.lua` file in the `plugins/` directory:

```lua
-- plugins/myfeature.lua
function my_custom_command()
    buffer.insert_string("Custom text!")
    buffer.insert_newline()
end

print("My plugin loaded!")
```

See [plugins/README.md](plugins/README.md) for complete API documentation.

## Building

```bash
make          # Release build (optimized)
make debug    # Debug build (with symbols)
make clean    # Clean build artifacts
```

## Architecture

OCCE uses a clean separation between core functionality (C) and features (Lua):

```
┌─────────────────────────────────────┐
│     OCCE Core (C - 43KB)           │
├─────────────────────────────────────┤
│ • Terminal I/O & Raw Mode          │
│ • Buffer Management & Undo/Redo   │
│ • Window System & Splitting        │
│ • Mouse Event Handling             │
│ • Git Integration                  │
│ • Lua VM & C-Lua Bridge           │
└─────────────────────────────────────┘
              ↕
┌─────────────────────────────────────┐
│    Plugin Layer (Lua)               │
├─────────────────────────────────────┤
│ • Text Editing & Visual Mode       │
│ • Syntax Highlighting (14 langs)   │
│ • Auto-Indent & Bracket Match      │
│ • Search & Replace                 │
│ • Custom Commands                  │
└─────────────────────────────────────┘
```

### Core Components

- **Terminal** - Raw mode, escape sequences, mouse events
- **Buffer** - Text storage, editing operations, undo/redo
- **Window** - Layout management, rendering, splitting
- **Git** - Repository detection, diff parsing, status display
- **Syntax** - Lua-based highlighting, cached rendering
- **Lua Bridge** - C ↔ Lua API, plugin system


## 📚 Documentation

- **[CHANGELOG.md](CHANGELOG.md)** - Project history and version changes
- **[plugins/README.md](plugins/README.md)** - Plugin API and development guide
- **[LICENSE](LICENSE)** - MIT License


## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

OCCE is free and open source software. Feel free to use, modify, and distribute it.

---

<div align="center">
**Built with ❤️ using C and Lua**

*Proving that great software doesn't need to be bloated*
</div>
