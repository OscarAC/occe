-- Safe Plugin Loader
-- This module provides robust plugin loading with error handling

local M = {}

-- Track loaded and failed plugins
M.loaded_plugins = {}
M.failed_plugins = {}

-- Safe plugin loader that isolates errors
function M.load_plugin_safe(filename)
    local success, error_msg = editor.load_plugin(filename)

    if success then
        table.insert(M.loaded_plugins, filename)
        return true
    else
        table.insert(M.failed_plugins, {
            name = filename,
            error = error_msg or "Unknown error"
        })

        -- Print error to console (for development)
        print(string.format("⚠ Plugin '%s' failed to load:", filename))
        if error_msg then
            -- Extract just the error message, not full path
            local short_error = error_msg:match(":%d+: (.+)") or error_msg
            print(string.format("  %s", short_error))
        end

        return false
    end
end

-- Load multiple plugins, continuing even if some fail
function M.load_plugins(plugin_list)
    for _, filename in ipairs(plugin_list) do
        M.load_plugin_safe(filename)
    end
end

-- Display loading summary
function M.print_summary()
    local total = #M.loaded_plugins + #M.failed_plugins

    if #M.failed_plugins > 0 then
        print(string.format("\n⚠ Plugin Loading Summary: %d/%d succeeded, %d failed",
            #M.loaded_plugins, total, #M.failed_plugins))

        print("\nFailed plugins:")
        for _, plugin in ipairs(M.failed_plugins) do
            print(string.format("  - %s", plugin.name))
        end
        print("")
    else
        -- Silent success - no need to spam the console
    end
end

-- Get list of loaded plugins
function M.get_loaded()
    return M.loaded_plugins
end

-- Get list of failed plugins
function M.get_failed()
    return M.failed_plugins
end

-- Check if a specific plugin loaded
function M.is_loaded(filename)
    for _, name in ipairs(M.loaded_plugins) do
        if name == filename then
            return true
        end
    end
    return false
end

return M
