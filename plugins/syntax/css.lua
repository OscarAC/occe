-- CSS syntax highlighting
local css_syntax = syntax.register("css")

syntax.add_extension(css_syntax, ".css")
syntax.set_comments(css_syntax, "/*", "/*", "*/")

-- Common CSS properties
local properties = {
    "color", "background", "background-color", "font-size", "font-family",
    "font-weight", "margin", "padding", "border", "width", "height",
    "display", "position", "top", "right", "bottom", "left", "flex",
    "grid", "text-align", "justify-content", "align-items", "opacity",
    "transform", "transition", "animation", "z-index", "overflow"
}

for _, prop in ipairs(properties) do
    syntax.add_keyword(css_syntax, prop, syntax.HL_KEYWORD)
end

-- Common values
local values = {
    "auto", "none", "inherit", "initial", "block", "inline", "flex",
    "grid", "absolute", "relative", "fixed", "sticky", "center",
    "left", "right", "top", "bottom", "hidden", "visible", "scroll"
}

for _, val in ipairs(values) do
    syntax.add_keyword(css_syntax, val, syntax.HL_CONSTANT)
end

-- Pseudo-classes
local pseudos = {"hover", "active", "focus", "visited", "first-child",
                 "last-child", "nth-child", "before", "after"}
for _, ps in ipairs(pseudos) do
    syntax.add_keyword(css_syntax, ps, syntax.HL_FUNCTION)
end
