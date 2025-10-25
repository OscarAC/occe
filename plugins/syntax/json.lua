-- JSON syntax highlighting
local json_syntax = syntax.register("json")

syntax.add_extension(json_syntax, ".json")
syntax.set_comments(json_syntax, "", "", "")  -- JSON has no comments

-- Constants
syntax.add_keyword(json_syntax, "true", syntax.HL_CONSTANT)
syntax.add_keyword(json_syntax, "false", syntax.HL_CONSTANT)
syntax.add_keyword(json_syntax, "null", syntax.HL_CONSTANT)
