-- OCCE Development Configuration
-- This file is used when running ./occe from the project directory

-- Load all syntax highlighting plugins
editor.load_plugin("syntax/lua.lua")
editor.load_plugin("syntax/c.lua")
editor.load_plugin("syntax/python.lua")
editor.load_plugin("syntax/javascript.lua")
editor.load_plugin("syntax/rust.lua")
editor.load_plugin("syntax/go.lua")
editor.load_plugin("syntax/java.lua")
editor.load_plugin("syntax/ruby.lua")
editor.load_plugin("syntax/typescript.lua")
editor.load_plugin("syntax/shell.lua")
editor.load_plugin("syntax/html.lua")
editor.load_plugin("syntax/css.lua")
editor.load_plugin("syntax/json.lua")
editor.load_plugin("syntax/markdown.lua")

-- Load core plugins
editor.load_plugin("core.lua")
editor.load_plugin("search.lua")

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
