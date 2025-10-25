-- Lua Syntax Highlighting
-- Registers syntax rules for Lua files

print("Loading Lua syntax highlighting...")

-- Register Lua syntax
local lua_syntax = syntax.register("lua")

-- Add file extensions
syntax.add_extension(lua_syntax, ".lua")

-- Set comment markers
syntax.set_comments(lua_syntax, "--", "--[[", "]]")

-- Lua keywords
local keywords = {
    "and", "break", "do", "else", "elseif",
    "end", "false", "for", "function", "if",
    "in", "local", "nil", "not", "or",
    "repeat", "return", "then", "true", "until", "while"
}

for _, kw in ipairs(keywords) do
    syntax.add_keyword(lua_syntax, kw, syntax.HL_KEYWORD)
end

-- Lua built-in functions
local builtins = {
    "print", "type", "pairs", "ipairs", "tonumber", "tostring",
    "assert", "error", "pcall", "xpcall", "require", "dofile",
    "loadfile", "setmetatable", "getmetatable", "rawget", "rawset",
    "next", "select", "unpack", "table", "string", "math", "io",
    "os", "debug", "coroutine", "package"
}

for _, fn in ipairs(builtins) do
    syntax.add_keyword(lua_syntax, fn, syntax.HL_FUNCTION)
end

-- Lua standard library modules
local modules = {
    "table", "string", "math", "io", "os", "debug", "coroutine", "package"
}

for _, mod in ipairs(modules) do
    syntax.add_keyword(lua_syntax, mod, syntax.HL_TYPE)
end

print("Lua syntax highlighting loaded!")
