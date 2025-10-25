-- TypeScript syntax highlighting
local ts_syntax = syntax.register("typescript")

syntax.add_extension(ts_syntax, ".ts")
syntax.add_extension(ts_syntax, ".tsx")
syntax.set_comments(ts_syntax, "//", "/*", "*/")

-- Keywords (JavaScript + TypeScript)
local keywords = {
    "abstract", "any", "as", "async", "await", "boolean", "break",
    "case", "catch", "class", "const", "continue", "debugger",
    "declare", "default", "delete", "do", "else", "enum", "export",
    "extends", "false", "finally", "for", "from", "function", "get",
    "if", "implements", "import", "in", "instanceof", "interface",
    "is", "keyof", "let", "module", "namespace", "never", "new",
    "null", "number", "of", "package", "private", "protected",
    "public", "readonly", "require", "return", "set", "static",
    "string", "super", "switch", "symbol", "this", "throw", "true",
    "try", "type", "typeof", "undefined", "var", "void", "while",
    "with", "yield"
}

for _, kw in ipairs(keywords) do
    syntax.add_keyword(ts_syntax, kw, syntax.HL_KEYWORD)
end

-- Built-ins
local builtins = {"console", "Array", "Object", "String", "Number",
                  "Boolean", "Promise", "Map", "Set", "Date", "Math",
                  "JSON", "parseInt", "parseFloat", "setTimeout"}
for _, fn in ipairs(builtins) do
    syntax.add_keyword(ts_syntax, fn, syntax.HL_FUNCTION)
end
