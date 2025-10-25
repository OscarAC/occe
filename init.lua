-- OCCE Configuration File
-- This file is loaded at startup and can customize the editor

print("Loading OCCE configuration...")

-- Load additional plugins
-- dofile("plugins/search.lua")
-- dofile("plugins/keybindings.lua")

-- Example keybindings (uncomment to use)
--[[
-- Ctrl+F to search
editor.bind_key(6, editor.KMOD.NONE, "search_forward")

-- Alt+R to replace
editor.bind_key(string.byte('r'), editor.KMOD.ALT, "replace_text")
]]

-- Custom functions
function quick_comment()
    local x, y = buffer.get_cursor()
    buffer.set_cursor(0, y)
    buffer.insert_string("# ")
    buffer.set_cursor(x + 2, y)
    editor.message("Line commented")
end

-- Startup message
editor.message("OCCE ready! Press : for commands")

print("Configuration loaded successfully!")
