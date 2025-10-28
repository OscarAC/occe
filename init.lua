-- OCCE Development Configuration
-- This file is used when running ./occe from the project directory

-- Tab settings
editor.set_tab_width(4)       -- Number of spaces per tab (default: 4)
editor.set_use_spaces(true)   -- Use spaces instead of tabs (default: true)

-- Load the safe plugin loader first
local loader = dofile("./plugins/core/plugin_loader.lua")

-- Load core plugins (essential key bindings and utilities)
loader.load_plugins({
    "core/core.lua"
})

-- Load all syntax highlighting plugins
loader.load_plugins({
    "syntax/lua.lua",
    "syntax/c.lua",
    "syntax/python.lua",
    "syntax/javascript.lua",
    "syntax/rust.lua",
    "syntax/go.lua",
    "syntax/java.lua",
    "syntax/ruby.lua",
    "syntax/typescript.lua",
    "syntax/shell.lua",
    "syntax/html.lua",
    "syntax/css.lua",
    "syntax/json.lua",
    "syntax/markdown.lua"
})

-- Load feature plugins (optional functionality)
loader.load_plugins({
    "features/search.lua",
    "features/word_navigation.lua",
    "features/git.lua",
    "features/window_commands.lua",
    "features/buffer_list.lua",
    "features/layouts.lua",
    "features/session_manager.lua"
})

-- Show summary if any plugins failed
loader.print_summary()

-- Dev mode startup message
editor.message("OCCE [DEV MODE] - Using local plugins")
