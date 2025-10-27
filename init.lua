-- OCCE Development Configuration
-- This file is used when running ./occe from the project directory

-- Load the safe plugin loader first
local loader = dofile("./plugins/plugin_loader.lua")

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

-- Load core plugins
loader.load_plugins({
    "core.lua",
    "search.lua",
    "word_navigation.lua",
    "git.lua",
    "window_commands.lua",
    "buffer_list.lua",
    "layouts.lua",
    "session_manager.lua"
})

-- Show summary if any plugins failed
loader.print_summary()

-- Custom functions for development
function quick_comment()
    local x, y = buffer.get_cursor()
    buffer.set_cursor(0, y)
    buffer.insert_string("# ")
    buffer.set_cursor(x + 2, y)
    editor.message("Line commented")
end

-- Dev mode startup message
editor.message("OCCE [DEV MODE] - Using local plugins")
