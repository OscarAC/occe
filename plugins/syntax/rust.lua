-- Rust syntax highlighting
local rust_syntax = syntax.register("rust")

syntax.add_extension(rust_syntax, ".rs")
syntax.set_comments(rust_syntax, "//", "/*", "*/")

-- Keywords
local keywords = {
    "fn", "let", "mut", "const", "static", "if", "else", "match",
    "for", "while", "loop", "break", "continue", "return", "pub",
    "mod", "use", "crate", "impl", "trait", "struct", "enum",
    "type", "where", "as", "async", "await", "move", "ref",
    "unsafe", "extern", "in", "self", "Self", "super"
}

for _, kw in ipairs(keywords) do
    syntax.add_keyword(rust_syntax, kw, syntax.HL_KEYWORD)
end

-- Types
local types = {"i8", "i16", "i32", "i64", "i128", "isize",
               "u8", "u16", "u32", "u64", "u128", "usize",
               "f32", "f64", "bool", "char", "str", "String",
               "Vec", "Option", "Result", "Box", "Rc", "Arc"}

for _, ty in ipairs(types) do
    syntax.add_keyword(rust_syntax, ty, syntax.HL_TYPE)
end

-- Constants
syntax.add_keyword(rust_syntax, "true", syntax.HL_CONSTANT)
syntax.add_keyword(rust_syntax, "false", syntax.HL_CONSTANT)
syntax.add_keyword(rust_syntax, "None", syntax.HL_CONSTANT)
syntax.add_keyword(rust_syntax, "Some", syntax.HL_CONSTANT)

-- Macros (common ones)
local macros = {"println!", "print!", "vec!", "format!", "panic!", "assert!"}
for _, m in ipairs(macros) do
    syntax.add_keyword(rust_syntax, m, syntax.HL_FUNCTION)
end
