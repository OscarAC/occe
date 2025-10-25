-- Go syntax highlighting
local go_syntax = syntax.register("go")

syntax.add_extension(go_syntax, ".go")
syntax.set_comments(go_syntax, "//", "/*", "*/")

-- Keywords
local keywords = {
    "break", "case", "chan", "const", "continue", "default",
    "defer", "else", "fallthrough", "for", "func", "go", "goto",
    "if", "import", "interface", "map", "package", "range",
    "return", "select", "struct", "switch", "type", "var"
}

for _, kw in ipairs(keywords) do
    syntax.add_keyword(go_syntax, kw, syntax.HL_KEYWORD)
end

-- Types
local types = {
    "bool", "byte", "complex64", "complex128", "error", "float32",
    "float64", "int", "int8", "int16", "int32", "int64", "rune",
    "string", "uint", "uint8", "uint16", "uint32", "uint64", "uintptr"
}

for _, ty in ipairs(types) do
    syntax.add_keyword(go_syntax, ty, syntax.HL_TYPE)
end

-- Constants
syntax.add_keyword(go_syntax, "true", syntax.HL_CONSTANT)
syntax.add_keyword(go_syntax, "false", syntax.HL_CONSTANT)
syntax.add_keyword(go_syntax, "nil", syntax.HL_CONSTANT)
syntax.add_keyword(go_syntax, "iota", syntax.HL_CONSTANT)

-- Built-in functions
local builtins = {"append", "cap", "close", "complex", "copy", "delete",
                  "imag", "len", "make", "new", "panic", "print", "println",
                  "real", "recover"}
for _, fn in ipairs(builtins) do
    syntax.add_keyword(go_syntax, fn, syntax.HL_FUNCTION)
end
