-- Keybindings plugin for OCCE
-- Demonstrates the keybinding system

print("Keybindings Plugin: Loading...")

-- Define some useful functions

function comment_line()
    local x, y = buffer.get_cursor()
    buffer.set_cursor(0, y)
    buffer.insert_string("-- ")
    buffer.set_cursor(x + 3, y)
    editor.message("Line commented")
end

function insert_date()
    local date = os.date("%Y-%m-%d")
    buffer.insert_string(date)
    editor.message("Date inserted")
end

function insert_time()
    local time = os.date("%H:%M:%S")
    buffer.insert_string(time)
    editor.message("Time inserted")
end

function duplicate_current_line()
    local x, y = buffer.get_cursor()
    local line = buffer.get_line(y)

    if line then
        -- Go to end of line
        buffer.set_cursor(#line, y)
        -- Insert newline
        buffer.insert_newline()
        -- Insert the duplicated content
        buffer.insert_string(line)
        editor.message("Line duplicated")
    end
end

function save_and_message()
    if buffer.save() then
        editor.message("File saved successfully!")
    else
        editor.message("Error saving file")
    end
end

-- Example keybindings
-- Note: These are examples - in practice you'd bind to specific key codes

--[[ Example key bindings (uncomment to use):

-- Alt+D to insert date
editor.bind_key(string.byte('d'), editor.KMOD.ALT, "insert_date")

-- Alt+T to insert time
editor.bind_key(string.byte('t'), editor.KMOD.ALT, "insert_time")

-- Ctrl+D to duplicate line
editor.bind_key(4, editor.KMOD.NONE, "duplicate_current_line")  -- Ctrl+D is key code 4

-- Ctrl+/ to comment (Ctrl+/ is Ctrl+_, which is key code 31)
editor.bind_key(31, editor.KMOD.NONE, "comment_line")

]]

print("Keybindings Plugin: Loaded")
print("Available functions:")
print("  - comment_line()")
print("  - insert_date()")
print("  - insert_time()")
print("  - duplicate_current_line()")
print("  - save_and_message()")
print("")
print("Use :lua editor.bind_key(key, modifier, \"function_name\") to bind keys")
