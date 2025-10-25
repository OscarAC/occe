# Changelog

All notable changes to OCCE will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Planned Features
- LSP support (code completion, diagnostics, go-to-definition)
- Search highlighting (visual feedback for searches)
- Incremental search (search as you type)
- Plugin manager
- Theme system
- Multiple clipboards (registers)
- Jump list navigation

## [0.6.0] - Phase 6: Git Integration

### Added
- Git integration with repository detection
- Git diff gutter showing line status (added/modified/deleted)
- Branch name display in status bar
- Color-coded git indicators (green +, yellow ~, red -)
- Buffer list command (`:ls`, `:buffers`)
- Window management command (`:only`)

### Changed
- Line number gutter now includes git status
- Status bar shows current branch when in git repository

### Size Impact
- Binary: 239KB → 244KB (+5KB)

## [0.5.0] - Phase 5: Window Management & Mouse Support

### Added
- Window splitting (`:split`, `:vsplit` commands)
- Horizontal and vertical split support
- Window navigation system (Ctrl+W command mode)
- Window navigation keybindings (w/W/h/j/k/l)
- Full mouse support (SGR extended protocol)
- Mouse click to position cursor
- Click and drag selection
- Scroll wheel support
- Buffer management across windows

### Changed
- Window system enhanced with tree-based split management
- Terminal mouse tracking enabled by default

### Size Impact
- Binary: 239KB → 239KB (+0KB!)

## [0.4.0] - Phase 4: Advanced Editing

### Added
- Undo/redo system with unlimited history
- Visual selection mode (v key)
- Yank (copy) and paste operations
- Delete selection command
- Bracket matching (% key to jump)
- Auto-indent for new lines (10 languages)
- Line joining command (`:join()`)
- Indent/dedent commands (`:indent()`, `:dedent()`)
- Tab handling improvements

### Supported Auto-Indent Languages
- C, C++, Python, JavaScript, TypeScript
- Rust, Go, Java, Lua, Shell

### Changed
- Enhanced editing workflow with modern features
- Improved code editing ergonomics

### Size Impact
- Binary: 239KB → 239KB (+0KB!)

## [0.3.0] - Phase 3: Syntax Highlighting

### Added
- Lua-based syntax highlighting system
- Support for 14 programming languages
- Cached highlighting for performance
- Multiline comment support
- Line numbers with dynamic width
- Color-coded syntax elements

### Supported Languages
- Systems: C, C++, Rust, Go
- Scripting: Python, Lua, JavaScript, TypeScript, Ruby, Shell
- Web: HTML, CSS
- Data/Docs: JSON, Markdown

### Color Scheme
- Keywords: Bold cyan
- Types: Bold yellow
- Strings: Green
- Comments: Gray/dim
- Functions: Magenta
- Numbers: Yellow

### Size Impact
- Binary: 223KB → 239KB (+16KB)

## [0.2.0] - Phase 2: Command Mode & Keybindings

### Added
- Command mode (`:` to enter)
- Lua command execution
- Built-in commands (`:w`, `:q`, `:wq`, etc.)
- Keybinding system
- Search and replace functionality
- Status message system
- Configuration system (init.lua)
- Example plugins

### Changed
- Enhanced terminal I/O with command mode
- Added status bar for messages and feedback

## [0.1.0] - Phase 1: Core Engine

### Added
- Terminal I/O with raw mode support
- Buffer management system
- Window rendering system
- Lua VM integration
- C ↔ Lua bridge
- Basic text editing (insert, delete, newline)
- File open and save operations
- Cursor movement (arrows, hjkl)
- Keyboard input handling

### Performance Metrics
- Binary size: 223KB
- Startup time: ~20ms
- Memory usage: ~3MB
- Input latency: ~5ms

### Architecture
- Clean separation between core (C) and features (Lua)
- Plugin-first architecture
- Efficient text buffer implementation
- Window system foundation

---

## Project Milestones

### Binary Size Evolution
- Phase 1: 223KB (Core engine)
- Phase 2: 223KB (Command mode, +0KB)
- Phase 3: 239KB (Syntax highlighting, +16KB)
- Phase 4: 239KB (Undo/redo & editing, +0KB)
- Phase 5: 239KB (Windows & mouse, +0KB)
- Phase 6: 244KB (Git integration, +5KB)

### Feature Count Evolution
- Phase 1: Basic editing
- Phase 2: +Command mode, keybindings, search
- Phase 3: +Syntax highlighting (14 languages)
- Phase 4: +Undo/redo, visual mode, auto-indent
- Phase 5: +Window splitting, mouse support
- Phase 6: +Git integration, buffer management

**Total**: Professional IDE features in 244KB!
