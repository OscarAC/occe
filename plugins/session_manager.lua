-- Session Manager Plugin
-- Manages window sessions with save/restore functionality

local session_file = os.getenv("HOME") .. "/.occe/session.lua"
local auto_save_enabled = false

-- Initialize session directory
local function ensure_session_dir()
    local config_dir = os.getenv("HOME") .. "/.occe"
    os.execute("mkdir -p " .. config_dir)
end

-- Serialize a Lua table to a string
local function serialize_table(tbl, indent)
    indent = indent or ""
    local result = "{\n"

    for k, v in pairs(tbl) do
        local key
        if type(k) == "string" then
            key = string.format('["%s"]', k)
        else
            key = string.format("[%d]", k)
        end

        local value
        if type(v) == "table" then
            value = serialize_table(v, indent .. "  ")
        elseif type(v) == "string" then
            value = string.format('"%s"', v)
        else
            value = tostring(v)
        end

        result = result .. indent .. "  " .. key .. " = " .. value .. ",\n"
    end

    result = result .. indent .. "}"
    return result
end

-- Save current window session
function save_window_session(filename)
    filename = filename or session_file
    ensure_session_dir()

    -- Get session data
    local session = window.serialize()

    -- Write to file
    local f = io.open(filename, "w")
    if not f then
        editor.message("Failed to save session to " .. filename)
        return false
    end

    f:write("-- OCCE Window Session\n")
    f:write("-- Saved: " .. os.date() .. "\n")
    f:write("-- Window count: " .. (session.window_count or 0) .. "\n\n")
    f:write("return " .. serialize_table(session) .. "\n")

    f:close()
    editor.message("Session saved to " .. filename)
    return true
end

-- Load window session from file
function load_window_session(filename)
    filename = filename or session_file

    local f = io.open(filename, "r")
    if not f then
        editor.message("No session file found at " .. filename)
        return false
    end
    f:close()

    -- Load the session file
    local session_loader = loadfile(filename)
    if not session_loader then
        editor.message("Failed to load session file")
        return false
    end

    local session = session_loader()
    if not session then
        editor.message("Invalid session data")
        return false
    end

    -- Restore windows (simplified - just show info for now)
    local count = session.window_count or 0
    editor.message("Session loaded: " .. count .. " windows")

    -- TODO: Actually restore the windows
    -- This would require creating splits and reopening files

    return true
end

-- Auto-save session on exit
function enable_auto_save()
    auto_save_enabled = true
    editor.message("Auto-save enabled")
end

function disable_auto_save()
    auto_save_enabled = false
    editor.message("Auto-save disabled")
end

-- Event hooks for session management
window.on_create(function(win_id)
    -- Optional: Auto-save when windows are created
    if auto_save_enabled then
        -- Could save here, but might be too frequent
    end
end)

window.on_close(function(win_id)
    -- Optional: Auto-save when windows close
    if auto_save_enabled then
        -- Could save here
    end
end)

-- Debug: Print session info
function print_session_info()
    local session = window.serialize()
    editor.message("Current session: " .. (session.window_count or 0) .. " windows")

    -- Print to console for debugging
    print("=== Window Session ===")
    print("Window count:", session.window_count)
    print("Active window:", session.active_window_id)

    if session.windows then
        for i, win in ipairs(session.windows) do
            print(string.format("  Window %d: id=%d pos=(%d,%d) size=%dx%d",
                i, win.id, win.x, win.y, win.width, win.height))
            if win.filename then
                print("    File:", win.filename)
            end
            if win.renderer then
                print("    Renderer:", win.renderer)
            end
        end
    end
    print("=====================")
end

-- Export functions
return {
    save = save_window_session,
    load = load_window_session,
    enable_auto_save = enable_auto_save,
    disable_auto_save = disable_auto_save,
    print_info = print_session_info
}
