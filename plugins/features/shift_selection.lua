-- Shift+Arrow Selection Plugin for OCCE
-- Allows text selection using Shift+Arrow keys

local M = {}

-- Move cursor left and update selection
function M.select_left()
    local x, y = buffer.get_cursor()

    -- Start selection if not already active
    if not buffer.has_selection() then
        buffer.start_selection()
    end

    -- Move cursor left
    if x > 0 then
        buffer.set_cursor(x - 1, y)
    elseif y > 0 then
        -- Move to end of previous line
        local prev_line = buffer.get_line(y - 1)
        if prev_line then
            buffer.set_cursor(#prev_line, y - 1)
        end
    end
end

-- Move cursor right and update selection
function M.select_right()
    local x, y = buffer.get_cursor()

    -- Start selection if not already active
    if not buffer.has_selection() then
        buffer.start_selection()
    end

    -- Move cursor right
    local line = buffer.get_line(y)
    if line then
        if x < #line then
            buffer.set_cursor(x + 1, y)
        else
            -- Move to start of next line
            local line_count = buffer.get_line_count()
            if y < line_count - 1 then
                buffer.set_cursor(0, y + 1)
            end
        end
    end
end

-- Move cursor up and update selection
function M.select_up()
    local x, y = buffer.get_cursor()

    -- Start selection if not already active
    if not buffer.has_selection() then
        buffer.start_selection()
    end

    -- Move cursor up
    if y > 0 then
        local new_y = y - 1
        local new_line = buffer.get_line(new_y)
        if new_line then
            -- Keep x position, but clamp to line length
            local new_x = x
            if new_x > #new_line then
                new_x = #new_line
            end
            buffer.set_cursor(new_x, new_y)
        end
    end
end

-- Move cursor down and update selection
function M.select_down()
    local x, y = buffer.get_cursor()

    -- Start selection if not already active
    if not buffer.has_selection() then
        buffer.start_selection()
    end

    -- Move cursor down
    local line_count = buffer.get_line_count()
    if y < line_count - 1 then
        local new_y = y + 1
        local new_line = buffer.get_line(new_y)
        if new_line then
            -- Keep x position, but clamp to line length
            local new_x = x
            if new_x > #new_line then
                new_x = #new_line
            end
            buffer.set_cursor(new_x, new_y)
        end
    end
end

-- Register keybindings
function M.setup()
    -- Export functions to global namespace so keybindings can find them
    _G.shift_select_left = M.select_left
    _G.shift_select_right = M.select_right
    _G.shift_select_up = M.select_up
    _G.shift_select_down = M.select_down

    -- Bind Shift+Arrow keys to selection functions
    editor.bind_key(editor.KEY.SHIFT_ARROW_LEFT, editor.KMOD.NONE, "shift_select_left")
    editor.bind_key(editor.KEY.SHIFT_ARROW_RIGHT, editor.KMOD.NONE, "shift_select_right")
    editor.bind_key(editor.KEY.SHIFT_ARROW_UP, editor.KMOD.NONE, "shift_select_up")
    editor.bind_key(editor.KEY.SHIFT_ARROW_DOWN, editor.KMOD.NONE, "shift_select_down")

    editor.message("Shift+Arrow selection plugin loaded")
end

-- Auto-setup on load
M.setup()

return M
