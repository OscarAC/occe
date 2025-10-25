-- JavaScript Syntax Highlighting

print("Loading JavaScript syntax highlighting...")

-- Register JavaScript syntax
local js_syntax = syntax.register("javascript")

-- Add file extensions
syntax.add_extension(js_syntax, ".js")
syntax.add_extension(js_syntax, ".mjs")
syntax.add_extension(js_syntax, ".jsx")

-- Set comment markers
syntax.set_comments(js_syntax, "//", "/*", "*/")

-- JavaScript keywords
local keywords = {
    "async", "await", "break", "case", "catch", "class",
    "const", "continue", "debugger", "default", "delete", "do",
    "else", "export", "extends", "finally", "for", "function",
    "if", "import", "in", "instanceof", "let", "new",
    "return", "static", "super", "switch", "this", "throw",
    "try", "typeof", "var", "void", "while", "with", "yield"
}

for _, kw in ipairs(keywords) do
    syntax.add_keyword(js_syntax, kw, syntax.HL_KEYWORD)
end

-- JavaScript built-ins
local builtins = {
    "console", "window", "document", "Array", "Object",
    "String", "Number", "Boolean", "Date", "Math",
    "JSON", "Promise", "Set", "Map", "WeakMap", "WeakSet",
    "Symbol", "Proxy", "Reflect", "parseInt", "parseFloat",
    "isNaN", "isFinite", "encodeURI", "decodeURI"
}

for _, fn in ipairs(builtins) do
    syntax.add_keyword(js_syntax, fn, syntax.HL_FUNCTION)
end

-- JavaScript constants
local constants = {
    "true", "false", "null", "undefined", "NaN", "Infinity"
}

for _, c in ipairs(constants) do
    syntax.add_keyword(js_syntax, c, syntax.HL_CONSTANT)
end

print("JavaScript syntax highlighting loaded!")
