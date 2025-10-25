-- HTML syntax highlighting
local html_syntax = syntax.register("html")

syntax.add_extension(html_syntax, ".html")
syntax.add_extension(html_syntax, ".htm")
syntax.set_comments(html_syntax, "<!--", "<!--", "-->")

-- HTML tags (common ones)
local tags = {
    "html", "head", "body", "title", "meta", "link", "script", "style",
    "div", "span", "p", "a", "img", "ul", "ol", "li", "table", "tr",
    "td", "th", "form", "input", "button", "select", "option", "textarea",
    "h1", "h2", "h3", "h4", "h5", "h6", "header", "footer", "nav",
    "section", "article", "aside", "main", "figure", "figcaption",
    "strong", "em", "code", "pre", "br", "hr"
}

for _, tag in ipairs(tags) do
    syntax.add_keyword(html_syntax, tag, syntax.HL_KEYWORD)
    syntax.add_keyword(html_syntax, "/" .. tag, syntax.HL_KEYWORD)
end

-- Common attributes
local attrs = {"class", "id", "href", "src", "alt", "title", "type",
               "name", "value", "placeholder", "required", "disabled"}
for _, attr in ipairs(attrs) do
    syntax.add_keyword(html_syntax, attr, syntax.HL_TYPE)
end
