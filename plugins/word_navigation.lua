-- Word Navigation Plugin for OCCE
-- Provides Ctrl+Arrow word-by-word movement
-- This demonstrates the plugin architecture

local M = {}

-- Helper: Check if character is a word character
local function is_word_char(ch)
    if not ch or ch == "" then
        return false
    end
    return ch:match("[%w_]") ~= nil
end

-- Helper: Check if character is whitespace
local function is_whitespace(ch)
    if not ch or ch == "" then
        return false
    end
    return ch:match("%s") ~= nil
end

-- Find the start position of the previous word
local function find_word_start(line, pos)
    if not line or line == "" or pos <= 0 then
        return 0
    end

    local x = pos

    -- If we're on whitespace, skip it
    while x > 0 do
        local ch = line:sub(x, x)
        if not is_whitespace(ch) then
            break
        end
        x = x - 1
    end

    -- Now skip the word characters
    while x > 0 do
        local ch = line:sub(x, x)
        if not is_word_char(ch) and not is_whitespace(ch) then
            -- We're on a symbol, skip symbols
            while x > 0 do
                local ch2 = line:sub(x, x)
                if is_word_char(ch2) or is_whitespace(ch2) then
                    break
                end
                x = x - 1
            end
            break
        elseif is_whitespace(ch) then
            x = x + 1
            break
        end
        x = x - 1
    end

    return math.max(0, x)
end

-- Find the end position of the next word
local function find_word_end(line, pos)
    if not line or line == "" then
        return 0
    end

    local len = #line
    local x = pos + 1  -- Start from next character

    -- Skip current word characters
    local in_word = false
    local in_symbol = false

    while x <= len do
        local ch = line:sub(x, x)

        if is_whitespace(ch) then
            if in_word or in_symbol then
                -- Found whitespace after a word/symbol
                -- Skip whitespace to find next word start
                while x <= len do
                    local ch2 = line:sub(x, x)
                    if not is_whitespace(ch2) then
                        return x - 1
                    end
                    x = x + 1
                end
                return len
            end
        elseif is_word_char(ch) then
            in_word = true
            in_symbol = false
        else
            -- Symbol character
            if in_word then
                -- Transition from word to symbol
                return x - 1
            end
            in_symbol = true
        end

        x = x + 1
    end

    return len
end

-- Move cursor to start of previous word
function M.move_word_left()
    local x, y = buffer.get_cursor()
    local line = buffer.get_line(y)

    if not line then
        return
    end

    if x == 0 then
        -- At start of line, move to end of previous line
        if y > 0 then
            local prev_line = buffer.get_line(y - 1)
            if prev_line then
                buffer.set_cursor(#prev_line, y - 1)
            end
        end
    else
        local new_x = find_word_start(line, x)
        buffer.set_cursor(new_x, y)
    end
end

-- Move cursor to start of next word
function M.move_word_right()
    local x, y = buffer.get_cursor()
    local line_count = buffer.get_line_count()
    local line = buffer.get_line(y)

    if not line then
        return
    end

    local line_len = #line

    if x >= line_len then
        -- At end of line, move to start of next line
        if y < line_count - 1 then
            buffer.set_cursor(0, y + 1)
        end
    else
        local new_x = find_word_end(line, x)
        buffer.set_cursor(new_x, y)
    end
end

-- Register keybindings
function M.setup()
    -- Bind Ctrl+Left Arrow to move word left
    editor.bind_key(editor.KEY.CTRL_ARROW_LEFT, editor.KMOD.NONE, "word_nav_left")

    -- Bind Ctrl+Right Arrow to move word right
    editor.bind_key(editor.KEY.CTRL_ARROW_RIGHT, editor.KMOD.NONE, "word_nav_right")

    editor.message("Word navigation plugin loaded")
end

-- Export functions to global namespace so keybindings can find them
_G.word_nav_left = M.move_word_left
_G.word_nav_right = M.move_word_right

-- Auto-setup on load
M.setup()

return M
