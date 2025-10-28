-- Git Integration Plugin for OCCE
-- Pure Lua implementation using process.execute() API
-- This demonstrates moving hard-coded C features to plugins!

local M = {}

-- Cache for git information (per buffer)
local git_cache = {}

-- Helper: Execute git command in a directory
local function run_git_command(repo_path, cmd)
    if not repo_path then
        return nil
    end

    -- Security: Reject paths with single quotes
    if repo_path:find("'") then
        return nil
    end

    local full_cmd = string.format("cd '%s' && git %s 2>/dev/null", repo_path, cmd)
    local stdout, stderr, exitcode = process.execute(full_cmd)

    if exitcode == 0 and stdout then
        return stdout
    end

    return nil
end

-- Find git repository root by walking up directory tree
function M.find_root(path)
    if not path or path == "" then
        return nil
    end

    -- Check if this is a file - extract directory
    local is_file_cmd = string.format("test -f '%s' && echo 'file' || echo 'dir'", path)
    local result = process.execute(is_file_cmd)
    if result and result:match("file") then
        -- Get directory name
        local dir_cmd = string.format("dirname '%s'", path)
        local dir = process.execute(dir_cmd)
        if dir then
            path = dir:gsub("\n$", "")
        end
    end

    -- Walk up directory tree looking for .git
    local test_path = path
    while test_path and test_path ~= "/" and test_path ~= "" do
        local git_dir = test_path .. "/.git"
        local check_cmd = string.format("test -e '%s' && echo 'found' || echo 'not found'", git_dir)
        local found = process.execute(check_cmd)

        if found and found:match("found") then
            return test_path
        end

        -- Go up one level
        local parent_cmd = string.format("dirname '%s'", test_path)
        local parent = process.execute(parent_cmd)
        if parent then
            parent = parent:gsub("\n$", "")
            if parent == test_path then
                break  -- Reached root
            end
            test_path = parent
        else
            break
        end
    end

    return nil
end

-- Check if path is in a git repository
function M.is_repo(path)
    local root = M.find_root(path)
    return root ~= nil
end

-- Get current branch name
function M.get_branch(repo_path)
    local output = run_git_command(repo_path, "branch --show-current")
    if output then
        -- Remove trailing newline
        return output:gsub("\n$", "")
    end
    return nil
end

-- Get file status (M=modified, A=added, D=deleted, etc.)
function M.get_status(repo_path)
    local output = run_git_command(repo_path, "status --porcelain")
    if not output or output == "" then
        return {}
    end

    local statuses = {}
    for line in output:gmatch("[^\n]+") do
        if #line > 3 then
            local status = line:sub(2, 2)
            if status == " " then
                status = line:sub(1, 1)
            end
            local filename = line:sub(4)
            statuses[filename] = status
        end
    end

    return statuses
end

-- Get per-line diff for a file
function M.get_file_diff(repo_path, filename)
    if not repo_path or not filename then
        return nil
    end

    -- Get file line count
    local file_path = repo_path .. "/" .. filename
    local count_cmd = string.format("wc -l < '%s' 2>/dev/null || echo 0", file_path)
    local count_str = process.execute(count_cmd)
    local num_lines = tonumber(count_str) or 0

    if num_lines == 0 then
        return nil
    end

    -- Initialize all lines as unchanged
    local line_statuses = {}
    for i = 1, num_lines do
        line_statuses[i] = "unchanged"
    end

    -- Get git diff
    local diff_output = run_git_command(repo_path, string.format("diff HEAD -- '%s'", filename))

    if not diff_output or diff_output == "" then
        -- No diff - check if file is tracked
        local tracked = run_git_command(repo_path, string.format("ls-files '%s'", filename))
        if not tracked or tracked == "" then
            -- File is new (untracked)
            for i = 1, num_lines do
                line_statuses[i] = "added"
            end
        end
        -- else file is tracked but unchanged
        return line_statuses
    end

    -- Parse diff output
    -- Format: @@ -old_start,old_count +new_start,new_count @@
    local current_line = 0
    local in_hunk = false

    for line in diff_output:gmatch("[^\n]+") do
        if line:match("^@@") then
            -- Parse hunk header
            local new_start, new_count = line:match("@@ %-(%d+),(%d+) %+(%d+),(%d+) @@")
            if new_start then
                current_line = tonumber(new_start) - 1  -- Convert to 0-based
                in_hunk = true
            else
                -- Try simpler format: @@ -start +start @@
                new_start = line:match("@@ [^ ]+ %+(%d+) @@")
                if new_start then
                    current_line = tonumber(new_start) - 1
                    in_hunk = true
                end
            end
        elseif in_hunk and #line > 0 then
            local marker = line:sub(1, 1)
            if marker == "+" then
                -- Added line
                current_line = current_line + 1
                if current_line <= num_lines then
                    line_statuses[current_line] = "added"
                end
            elseif marker == "-" then
                -- Deleted line (doesn't exist in new file)
                -- Don't advance current_line
            elseif marker == " " then
                -- Unchanged line
                current_line = current_line + 1
            end
        end
    end

    return line_statuses
end

-- Initialize git for a buffer
function M.init_buffer(filename)
    if not filename then
        return nil
    end

    local root = M.find_root(filename)
    if not root then
        git_cache[filename] = nil
        return nil
    end

    -- Get relative path from repo root
    local rel_path = filename
    if filename:sub(1, #root) == root then
        rel_path = filename:sub(#root + 2)  -- +2 to skip the /
    end

    local cache = {
        root = root,
        branch = M.get_branch(root),
        rel_path = rel_path,
        diff = M.get_file_diff(root, rel_path)
    }

    git_cache[filename] = cache
    return cache
end

-- Get cached git info for buffer
function M.get_cache(filename)
    if not filename then
        return nil
    end

    local cache = git_cache[filename]
    if not cache then
        -- Try to initialize
        cache = M.init_buffer(filename)
    end

    return cache
end

-- Refresh git diff for current buffer
function M.refresh()
    local filename = buffer.get_filename()
    if filename then
        git_cache[filename] = nil
        return M.init_buffer(filename)
    end
    return nil
end

-- Gutter renderer function (called for each line)
function M.render_gutter(line_num)
    local filename = buffer.get_filename()
    if not filename then
        return "  "
    end

    local cache = M.get_cache(filename)
    if not cache or not cache.diff then
        return "  "
    end

    -- line_num is 0-based in C, convert to 1-based for Lua
    local lua_line = line_num + 1
    local status = cache.diff[lua_line]

    if status == "added" then
        return "\x1b[32m+\x1b[0m "  -- Green +
    elseif status == "modified" then
        return "\x1b[33m~\x1b[0m "  -- Yellow ~
    elseif status == "deleted" then
        return "\x1b[31m-\x1b[0m "  -- Red -
    else
        return "  "
    end
end

-- Setup function
function M.setup()
    -- Register gutter renderer
    _G._gutter_renderer = M.render_gutter

    -- Initialize git for current buffer (if buffer API is available and has filename)
    if buffer and buffer.get_filename then
        local success, filename = pcall(buffer.get_filename)
        if success and filename then
            M.init_buffer(filename)
        end
    end

    editor.message("Git plugin loaded")
end

-- Auto-setup on load
M.setup()

return M
