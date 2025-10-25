-- Simple Hello World plugin for OCCE
-- Demonstrates basic buffer manipulation

print("Hello Plugin: Loaded!")

-- Function to insert a greeting
function say_hello()
    local x, y = buffer.get_cursor()
    buffer.insert_string("Hello from Lua plugin!")
    buffer.insert_newline()
    editor.message("Greeting inserted!")
end

-- Function to insert current timestamp
function insert_timestamp()
    local timestamp = os.date("%Y-%m-%d %H:%M:%S")
    buffer.insert_string(timestamp)
end

-- Function to insert a separator line
function insert_separator()
    buffer.insert_newline()
    buffer.insert_string(string.rep("-", 60))
    buffer.insert_newline()
end

print("Hello Plugin: Functions registered (say_hello, insert_timestamp, insert_separator)")
