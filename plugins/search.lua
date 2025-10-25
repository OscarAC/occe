-- Search plugin for OCCE
-- Provides search and replace functionality

print("Search Plugin: Loading...")

-- Search forward from current position
function search_forward()
    -- In a full implementation, this would open a prompt
    -- For now, it's called from command mode with the query as an argument
    editor.message("Use :lua search_for(\"text\") to search")
end

-- Search for specific text
function search_for(text)
    if not text or text == "" then
        editor.message("Please provide search text")
        return
    end

    local row, col = buffer.search(text, true)
    if row then
        buffer.set_cursor(col, row)
        editor.message(string.format("Found at %d:%d", row + 1, col + 1))
    else
        editor.message("Not found: " .. text)
    end
end

-- Search backward for specific text
function search_back(text)
    if not text or text == "" then
        editor.message("Please provide search text")
        return
    end

    local row, col = buffer.search(text, false)
    if row then
        buffer.set_cursor(col, row)
        editor.message(string.format("Found at %d:%d", row + 1, col + 1))
    else
        editor.message("Not found: " .. text)
    end
end

-- Replace text
function replace_text(search, replace_with)
    if not search or search == "" then
        editor.message("Please provide search text")
        return
    end

    replace_with = replace_with or ""
    local count = buffer.replace(search, replace_with, false)

    if count > 0 then
        editor.message(string.format("Replaced %d occurrence(s)", count))
    else
        editor.message("No occurrences found")
    end
end

-- Replace all occurrences
function replace_all(search, replace_with)
    if not search or search == "" then
        editor.message("Please provide search text")
        return
    end

    replace_with = replace_with or ""
    local count = buffer.replace(search, replace_with, true)

    if count > 0 then
        editor.message(string.format("Replaced %d occurrence(s)", count))
    else
        editor.message("No occurrences found")
    end
end

print("Search Plugin: Loaded")
print("Available functions:")
print("  - search_for(\"text\")        - Search forward")
print("  - search_back(\"text\")       - Search backward")
print("  - replace_text(\"old\", \"new\") - Replace once")
print("  - replace_all(\"old\", \"new\")  - Replace all")
print("")
print("Example: :search_for(\"TODO\")")
