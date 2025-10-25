-- Java syntax highlighting
local java_syntax = syntax.register("java")

syntax.add_extension(java_syntax, ".java")
syntax.set_comments(java_syntax, "//", "/*", "*/")

-- Keywords
local keywords = {
    "abstract", "assert", "boolean", "break", "byte", "case", "catch",
    "char", "class", "const", "continue", "default", "do", "double",
    "else", "enum", "extends", "final", "finally", "float", "for",
    "goto", "if", "implements", "import", "instanceof", "int",
    "interface", "long", "native", "new", "package", "private",
    "protected", "public", "return", "short", "static", "strictfp",
    "super", "switch", "synchronized", "this", "throw", "throws",
    "transient", "try", "void", "volatile", "while"
}

for _, kw in ipairs(keywords) do
    syntax.add_keyword(java_syntax, kw, syntax.HL_KEYWORD)
end

-- Types
local types = {
    "Boolean", "Byte", "Character", "Double", "Float", "Integer",
    "Long", "Short", "String", "Object", "Thread", "Runnable",
    "List", "ArrayList", "Map", "HashMap", "Set", "HashSet"
}

for _, ty in ipairs(types) do
    syntax.add_keyword(java_syntax, ty, syntax.HL_TYPE)
end

-- Constants
syntax.add_keyword(java_syntax, "true", syntax.HL_CONSTANT)
syntax.add_keyword(java_syntax, "false", syntax.HL_CONSTANT)
syntax.add_keyword(java_syntax, "null", syntax.HL_CONSTANT)
