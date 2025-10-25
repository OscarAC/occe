-- C/C++ Syntax Highlighting

print("Loading C/C++ syntax highlighting...")

-- Register C syntax
local c_syntax = syntax.register("c")

-- Add file extensions
syntax.add_extension(c_syntax, ".c")
syntax.add_extension(c_syntax, ".h")
syntax.add_extension(c_syntax, ".cpp")
syntax.add_extension(c_syntax, ".hpp")
syntax.add_extension(c_syntax, ".cc")
syntax.add_extension(c_syntax, ".cxx")

-- Set comment markers
syntax.set_comments(c_syntax, "//", "/*", "*/")

-- C/C++ keywords
local keywords = {
    "auto", "break", "case", "char", "const", "continue",
    "default", "do", "double", "else", "enum", "extern",
    "float", "for", "goto", "if", "int", "long",
    "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void",
    "volatile", "while",
    -- C++ additions
    "class", "namespace", "template", "typename", "public",
    "private", "protected", "virtual", "override", "final",
    "try", "catch", "throw", "new", "delete", "this",
    "bool", "true", "false", "nullptr", "constexpr"
}

for _, kw in ipairs(keywords) do
    syntax.add_keyword(c_syntax, kw, syntax.HL_KEYWORD)
end

-- C/C++ types
local types = {
    "int8_t", "int16_t", "int32_t", "int64_t",
    "uint8_t", "uint16_t", "uint32_t", "uint64_t",
    "size_t", "ssize_t", "ptrdiff_t", "intptr_t",
    "FILE", "NULL", "bool"
}

for _, t in ipairs(types) do
    syntax.add_keyword(c_syntax, t, syntax.HL_TYPE)
end

-- Preprocessor directives
local preprocessor = {
    "#include", "#define", "#if", "#ifdef", "#ifndef",
    "#else", "#elif", "#endif", "#undef", "#pragma"
}

for _, pp in ipairs(preprocessor) do
    syntax.add_keyword(c_syntax, pp, syntax.HL_PREPROCESSOR)
end

print("C/C++ syntax highlighting loaded!")
