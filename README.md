# OCCE - Optimized Console Code Editor

> A professional-grade terminal text editor proving that modern IDE features don't require bloat.

**OCCE** is a lightweight, fast, and feature-rich code editor built with C and Lua. It combines the simplicity of traditional terminal editors with modern conveniences like syntax highlighting, git integration, and mouse support - all in a **244KB binary**.

---

**Quick Links:** [Features](#-features) • [Quick Start](#quick-start) • [Controls](#controls) • [Commands](#commands) • [Contributing](#-contributing)

---

## Why OCCE?

- **Tiny but Powerful**: Full IDE features in 244KB (compare: VS Code ~100MB, Sublime ~20MB)
- **Blazing Fast**: 20ms startup, 5ms input latency, 3MB RAM usage
- **Modern Features**: Syntax highlighting, git integration, window splitting, mouse support
- **Extensible**: Lua plugin system for custom functionality
- **Zero Dependencies**: Statically linked, runs anywhere (just needs Lua at build time)

## ✨ Features

- ⚡ **Lightning-fast C core** - 244KB binary, 20ms startup
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
│     OCCE Core (C - 244KB)          │
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

## Performance

| Metric | Target | Actual |
|--------|--------|--------|
| Binary Size | < 500KB | 244KB ✓ |
| Startup Time | < 50ms | ~20ms ✓ |
| Memory Usage | < 10MB | ~3MB ✓ |
| Input Latency | < 16ms | ~5ms ✓ |

**Achievement**: All professional IDE features fit in just 244KB!

## 📚 Documentation

- **[CHANGELOG.md](CHANGELOG.md)** - Project history and version changes
- **[plugins/README.md](plugins/README.md)** - Plugin API and development guide
- **[LICENSE](LICENSE)** - MIT License

## 🎯 Project Goals

OCCE demonstrates that professional development tools don't need to be bloated:
- **Performance First** - Written in C11 for maximum speed and minimal size
- **Clean Architecture** - Clear separation between core (C) and features (Lua)
- **Zero Bloat** - Every feature must justify its size impact
- **Developer Friendly** - Simple, readable code that's easy to understand and extend

## 🗺️ Roadmap

### Current Status: v0.6.0
All core features complete! OCCE is fully usable for daily coding work.

### Planned Features
- **LSP support** - Code completion, diagnostics, go-to-definition
- **Search highlighting** - Visual feedback for search matches
- **Incremental search** - Search as you type
- **Plugin manager** - Easy plugin installation and management
- **Theme system** - Customizable color schemes
- **More languages** - Expand syntax highlighting support

See [CHANGELOG.md](CHANGELOG.md) for complete project history.

## 🤝 Contributing

Contributions are welcome! OCCE is designed to be hackable and easy to understand.

### Ways to Contribute
- 🐛 **Report bugs** - Open an issue with reproduction steps
- 💡 **Suggest features** - Share ideas for improvements
- 🔌 **Create plugins** - Extend OCCE with Lua plugins
- 📝 **Improve docs** - Help make documentation clearer
- 🎨 **Add syntax definitions** - Support more languages
- 💻 **Submit code** - Fix bugs or implement features

### Getting Started
1. Fork the repository
2. Check [plugins/README.md](plugins/README.md) for plugin development
3. Review the source code in `src/` and `include/`
4. Browse open issues for ideas
5. Submit a pull request!

### Code Style
- Follow existing C11 style conventions
- Keep functions small and focused
- Comment complex logic clearly
- Test your changes thoroughly
- Ensure no compiler warnings

### Project Structure
```
occe/
├── src/           # C source files
├── include/       # C header files
├── plugins/       # Lua plugins and syntax definitions
├── init.lua       # Default configuration
├── Makefile       # Build system
└── build_lua.sh   # Bundled Lua builder
```

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

OCCE is free and open source software. Feel free to use, modify, and distribute it.

---

<div align="center">

**Built with ❤️ using C and Lua**

*Proving that great software doesn't need to be bloated*

[⭐ Star this repo](../../stargazers) • [🐛 Report Bug](../../issues) • [💡 Request Feature](../../issues)

</div>
