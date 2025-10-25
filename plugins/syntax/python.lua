-- Python Syntax Highlighting

print("Loading Python syntax highlighting...")

-- Register Python syntax
local py_syntax = syntax.register("python")

-- Add file extensions
syntax.add_extension(py_syntax, ".py")
syntax.add_extension(py_syntax, ".pyw")

-- Set comment markers (Python only has single-line comments)
syntax.set_comments(py_syntax, "#", nil, nil)

-- Python keywords
local keywords = {
    "and", "as", "assert", "async", "await", "break",
    "class", "continue", "def", "del", "elif", "else",
    "except", "False", "finally", "for", "from", "global",
    "if", "import", "in", "is", "lambda", "None",
    "nonlocal", "not", "or", "pass", "raise", "return",
    "True", "try", "while", "with", "yield"
}

for _, kw in ipairs(keywords) do
    syntax.add_keyword(py_syntax, kw, syntax.HL_KEYWORD)
end

-- Python built-in functions
local builtins = {
    "abs", "all", "any", "ascii", "bin", "bool",
    "breakpoint", "bytearray", "bytes", "callable", "chr",
    "classmethod", "compile", "complex", "delattr", "dict",
    "dir", "divmod", "enumerate", "eval", "exec",
    "filter", "float", "format", "frozenset", "getattr",
    "globals", "hasattr", "hash", "help", "hex",
    "id", "input", "int", "isinstance", "issubclass",
    "iter", "len", "list", "locals", "map",
    "max", "memoryview", "min", "next", "object",
    "oct", "open", "ord", "pow", "print",
    "property", "range", "repr", "reversed", "round",
    "set", "setattr", "slice", "sorted", "staticmethod",
    "str", "sum", "super", "tuple", "type",
    "vars", "zip"
}

for _, fn in ipairs(builtins) do
    syntax.add_keyword(py_syntax, fn, syntax.HL_FUNCTION)
end

-- Python constants
local constants = {
    "True", "False", "None"
}

for _, c in ipairs(constants) do
    syntax.add_keyword(py_syntax, c, syntax.HL_CONSTANT)
end

print("Python syntax highlighting loaded!")
