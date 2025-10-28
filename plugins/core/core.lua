-- Core OCCE plugin
-- This plugin sets up essential key bindings and provides utilities

-- Welcome message
print("OCCE Core Plugin Loaded")

-- ============================================================================
-- Key Bindings
-- ============================================================================

-- File operations
editor.bind_key(editor.KEY.CTRL_S, editor.KMOD.NONE, "editor.save")
editor.bind_key(editor.KEY.CTRL_Q, editor.KMOD.NONE, "editor.quit")

-- Undo/Redo
editor.bind_key(editor.KEY.CTRL_Z, editor.KMOD.NONE, "editor.undo")
editor.bind_key(editor.KEY.CTRL_R, editor.KMOD.NONE, "editor.redo")

-- Clipboard
editor.bind_key(editor.KEY.CTRL_C, editor.KMOD.NONE, "editor.copy")
editor.bind_key(editor.KEY.CTRL_V, editor.KMOD.NONE, "editor.paste")

-- Tab navigation
editor.bind_key(editor.KEY.CTRL_PAGE_DOWN, editor.KMOD.NONE, "editor.tabnext")
editor.bind_key(editor.KEY.CTRL_PAGE_UP, editor.KMOD.NONE, "editor.tabprev")

-- Window navigation
editor.bind_key(editor.KEY.CTRL_W, editor.KMOD.NONE, "editor.window_next")

-- Helper function to insert text at cursor
function insert_text(text)
    buffer.insert_string(text)
end

-- Function to duplicate current line
function duplicate_line()
    local x, y = buffer.get_cursor()
    local line = buffer.get_line(y)

    if line then
        -- Go to end of line
        buffer.set_cursor(#line, y)
        -- Insert newline
        buffer.insert_newline()
        -- Insert the duplicated content
        buffer.insert_string(line)
    end
end

-- Function to delete current line
function delete_line()
    local x, y = buffer.get_cursor()
    local line = buffer.get_line(y)

    if line then
        -- Go to start of line
        buffer.set_cursor(0, y)
        -- Delete all characters
        for i = 1, #line do
            buffer.delete_char()
        end
        -- Delete the newline (join with next line or delete empty line)
        buffer.delete_char()
    end
end

-- Function to show buffer info
function buffer_info()
    local x, y = buffer.get_cursor()
    local line_count = buffer.get_line_count()

    print(string.format("Cursor: %d,%d | Lines: %d", x, y, line_count))
end

-- Export functions for use by other plugins
return {
    insert_text = insert_text,
    duplicate_line = duplicate_line,
    delete_line = delete_line,
    buffer_info = buffer_info
}
