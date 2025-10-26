-- Window Commands Plugin
-- Provides remappable window management commands

-- Window command mode state
local window_mode_active = false

-- Enter window command mode
function enter_window_mode()
    window_mode_active = true
    editor.message("-- WINDOW --")
end

-- Exit window command mode
function exit_window_mode()
    window_mode_active = false
    editor.message("")
end

-- Window command dispatcher
-- This function is called when in window mode to handle key presses
function handle_window_command(key)
    if not window_mode_active then
        return false
    end

    -- Convert key to character
    local char = string.char(key)

    if char == 'w' then
        -- Next window
        window.next()
        exit_window_mode()
        return true

    elseif char == 'W' then
        -- Previous window
        window.prev()
        exit_window_mode()
        return true

    elseif char == 'h' then
        -- Focus left window
        window.focus_direction("left")
        exit_window_mode()
        return true

    elseif char == 'j' then
        -- Focus down window
        window.focus_direction("down")
        exit_window_mode()
        return true

    elseif char == 'k' then
        -- Focus up window
        window.focus_direction("up")
        exit_window_mode()
        return true

    elseif char == 'l' then
        -- Focus right window
        window.focus_direction("right")
        exit_window_mode()
        return true

    elseif char == 'q' then
        -- Close current window
        window.close()
        exit_window_mode()
        return true

    elseif char == 'o' then
        -- Only - close all other windows
        window.only()
        exit_window_mode()
        return true

    elseif char == '=' then
        -- Equalize window sizes
        window.equalize()
        editor.message("Windows equalized")
        exit_window_mode()
        return true

    elseif char == 's' then
        -- Horizontal split (TODO: needs split creation API)
        editor.message("Split creation not yet implemented")
        exit_window_mode()
        return true

    elseif char == 'v' then
        -- Vertical split (TODO: needs split creation API)
        editor.message("Vertical split creation not yet implemented")
        exit_window_mode()
        return true

    else
        -- Unknown command, exit window mode
        exit_window_mode()
        return false
    end
end

-- Bind Ctrl+W to enter window mode
editor.bind_key(string.byte('W'), editor.KMOD.CTRL, "enter_window_mode")

-- Debug: print window info
function window_info()
    local win_id = window.get_current()
    if win_id then
        local info = window.get_info(win_id)
        if info then
            local msg = string.format("Window %d: %dx%d at (%d,%d) type=%s",
                info.id, info.width, info.height, info.x, info.y, info.type)
            editor.message(msg)
        end
    else
        editor.message("No active window")
    end
end

-- Export functions
return {
    enter_window_mode = enter_window_mode,
    exit_window_mode = exit_window_mode,
    handle_window_command = handle_window_command,
    window_info = window_info,
    is_window_mode_active = function() return window_mode_active end
}
