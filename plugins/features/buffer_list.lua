-- Buffer List Custom Window Plugin
-- Demonstrates custom window rendering
-- Shows list of open buffers with navigation

-- Register the buffer list renderer
window.register_renderer("buffer_list", {
    -- Render function: called every frame
    render = function(data, x, y, width, height)
        -- Draw title
        terminal.move(y, x)
        terminal.set_color("\x1b[1;36m")  -- Bold cyan
        terminal.write("=== Buffer List ===")
        terminal.reset_color()
        terminal.clear_line()

        -- Get buffer count
        local buf_count = buffer.get_line_count()

        -- Draw instructions
        terminal.move(y + 1, x)
        terminal.set_color("\x1b[90m")  -- Gray
        terminal.write("Use j/k to navigate, Enter to select, q to close")
        terminal.reset_color()
        terminal.clear_line()

        -- Draw separator
        terminal.move(y + 2, x)
        for i = 1, width do
            terminal.write("-")
        end

        -- Draw buffer entries (placeholder data)
        local entries = data.entries or {"[Buffer 1]", "[Buffer 2]", "[Buffer 3]"}
        local selected = data.selected or 1

        for i, entry in ipairs(entries) do
            if y + 3 + i - 1 >= y + height - 1 then
                break  -- Don't overflow window
            end

            terminal.move(y + 3 + i - 1, x)

            -- Highlight selected entry
            if i == selected then
                terminal.set_color("\x1b[7m")  -- Reverse video
                terminal.write("> " .. entry)
                terminal.reset_color()
            else
                terminal.write("  " .. entry)
            end
            terminal.clear_line()
        end

        -- Clear remaining lines
        for i = #entries + 4, height - 1 do
            terminal.move(y + i, x)
            terminal.clear_line()
        end
    end,

    -- Key handler: called when key is pressed in this window
    on_key = function(data, key)
        local char = string.char(key)

        -- Initialize data if needed
        if not data.entries then
            data.entries = {"README.md", "init.lua", "plugins/core.lua"}
            data.selected = 1
        end

        if char == 'j' or key == 0x1B5B42 then  -- j or Down arrow
            data.selected = data.selected + 1
            if data.selected > #data.entries then
                data.selected = #data.entries
            end
            return true  -- Key handled

        elseif char == 'k' or key == 0x1B5B41 then  -- k or Up arrow
            data.selected = data.selected - 1
            if data.selected < 1 then
                data.selected = 1
            end
            return true

        elseif char == 'q' then
            -- Close this window (TODO: implement proper close)
            editor.message("Buffer list closed")
            return true

        elseif key == 10 or key == 13 then  -- Enter
            editor.message("Selected: " .. data.entries[data.selected])
            return true
        end

        return false  -- Key not handled
    end,

    -- Optional: Called when window gains focus
    on_focus = function(data)
        -- Refresh buffer list
        data.entries = {"README.md", "init.lua", "plugins/core.lua"}
    end,

    -- Optional: Called when window loses focus
    on_blur = function(data)
        -- Could save state here
    end
})

-- Function to open buffer list window
function open_buffer_list()
    local data = {
        entries = {"README.md", "init.lua", "plugins/core.lua"},
        selected = 1
    }

    local win_id = window.create_custom("buffer_list", data)
    if win_id then
        editor.message("Buffer list opened (window " .. win_id .. ")")
    else
        editor.message("Failed to create buffer list window")
    end
end

-- Bind F3 to open buffer list
-- editor.bind_key(0x1B5B3133, editor.KMOD.NONE, "open_buffer_list")  -- F3

editor.message("Buffer list plugin loaded! Use :open_buffer_list() to test")
