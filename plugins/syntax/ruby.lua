-- Ruby syntax highlighting
local ruby_syntax = syntax.register("ruby")

syntax.add_extension(ruby_syntax, ".rb")
syntax.set_comments(ruby_syntax, "#", "", "")

-- Keywords
local keywords = {
    "BEGIN", "END", "alias", "and", "begin", "break", "case",
    "class", "def", "defined?", "do", "else", "elsif", "end",
    "ensure", "for", "if", "in", "module", "next", "not", "or",
    "redo", "rescue", "retry", "return", "self", "super", "then",
    "undef", "unless", "until", "when", "while", "yield"
}

for _, kw in ipairs(keywords) do
    syntax.add_keyword(ruby_syntax, kw, syntax.HL_KEYWORD)
end

-- Constants
syntax.add_keyword(ruby_syntax, "true", syntax.HL_CONSTANT)
syntax.add_keyword(ruby_syntax, "false", syntax.HL_CONSTANT)
syntax.add_keyword(ruby_syntax, "nil", syntax.HL_CONSTANT)

-- Built-ins
local builtins = {"puts", "print", "gets", "require", "include",
                  "attr_reader", "attr_writer", "attr_accessor",
                  "private", "public", "protected"}
for _, fn in ipairs(builtins) do
    syntax.add_keyword(ruby_syntax, fn, syntax.HL_FUNCTION)
end
