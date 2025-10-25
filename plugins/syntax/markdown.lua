-- Markdown syntax highlighting
local md_syntax = syntax.register("markdown")

syntax.add_extension(md_syntax, ".md")
syntax.add_extension(md_syntax, ".markdown")
syntax.set_comments(md_syntax, "", "", "")

-- Headers
for i = 1, 6 do
    syntax.add_keyword(md_syntax, string.rep("#", i), syntax.HL_KEYWORD)
end

-- Emphasis markers
local markers = {"**", "*", "__", "_", "`", "```", "~~", "=="}
for _, m in ipairs(markers) do
    syntax.add_keyword(md_syntax, m, syntax.HL_TYPE)
end

-- List markers
syntax.add_keyword(md_syntax, "-", syntax.HL_KEYWORD)
syntax.add_keyword(md_syntax, "*", syntax.HL_KEYWORD)
syntax.add_keyword(md_syntax, "+", syntax.HL_KEYWORD)

-- Links/images
syntax.add_keyword(md_syntax, "[", syntax.HL_FUNCTION)
syntax.add_keyword(md_syntax, "]", syntax.HL_FUNCTION)
syntax.add_keyword(md_syntax, "(", syntax.HL_FUNCTION)
syntax.add_keyword(md_syntax, ")", syntax.HL_FUNCTION)
syntax.add_keyword(md_syntax, "!", syntax.HL_FUNCTION)
