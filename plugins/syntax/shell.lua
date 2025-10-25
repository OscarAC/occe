-- Shell script syntax highlighting
local sh_syntax = syntax.register("shell")

syntax.add_extension(sh_syntax, ".sh")
syntax.add_extension(sh_syntax, ".bash")
syntax.set_comments(sh_syntax, "#", "", "")

-- Keywords
local keywords = {
    "if", "then", "else", "elif", "fi", "case", "esac", "for",
    "select", "while", "until", "do", "done", "in", "function",
    "time", "coproc", "select", "trap", "return", "exit", "break",
    "continue", "local", "declare", "typeset", "readonly", "export"
}

for _, kw in ipairs(keywords) do
    syntax.add_keyword(sh_syntax, kw, syntax.HL_KEYWORD)
end

-- Built-in commands
local builtins = {"echo", "printf", "read", "cd", "pwd", "pushd", "popd",
                  "test", "eval", "exec", "source", "alias", "unalias",
                  "set", "unset", "shift", "let", "true", "false"}
for _, fn in ipairs(builtins) do
    syntax.add_keyword(sh_syntax, fn, syntax.HL_FUNCTION)
end
