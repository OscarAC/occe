-- OCCE Theme Manager Plugin
-- This plugin demonstrates how to use the theme system

print("OCCE Theme Manager Loaded")

-- List all available themes
function list_themes()
    local themes = theme.list()
    editor.message("Available themes:")
    for i, name in ipairs(themes) do
        local info = theme.get_info(name)
        local mode = info.dark_mode and "dark" or "light"
        local desc = info.description or "No description"
        print(string.format("  %d. %s (%s) - %s", i, name, mode, desc))
    end
end

-- Switch to a specific theme
function set_theme(theme_name)
    if theme.set_active(theme_name) then
        editor.message("Theme changed to: " .. theme_name)
    else
        editor.message("Theme not found: " .. theme_name)
    end
end

-- Get current active theme
function get_current_theme()
    local current = theme.get_active()
    if current then
        editor.message("Current theme: " .. current)
    else
        editor.message("No theme active")
    end
end

-- Example: Create a custom theme
function create_custom_theme()
    -- Create a new theme
    local my_theme = theme.create("my-custom-theme")

    -- Set colors using hex values (TrueColor)
    theme.set_color(my_theme, theme.HL_KEYWORD, theme.hex("#FF6B6B"), "black", {bold = true})
    theme.set_color(my_theme, theme.HL_STRING, theme.hex("#4ECDC4"), "black", {})
    theme.set_color(my_theme, theme.HL_NUMBER, theme.hex("#FFE66D"), "black", {})
    theme.set_color(my_theme, theme.HL_COMMENT, theme.hex("#95A5A6"), "black", {italic = true})
    theme.set_color(my_theme, theme.HL_FUNCTION, theme.hex("#A8E6CF"), "black", {})

    -- Or use ANSI color names for terminal compatibility
    theme.set_color(my_theme, theme.HL_TYPE, "cyan", "black", {})
    theme.set_color(my_theme, theme.HL_OPERATOR, "white", "black", {})

    -- Register the theme so it can be used
    theme.register(my_theme)

    editor.message("Custom theme 'my-custom-theme' created!")
end

-- Example: Create a theme with RGB values
function create_rgb_theme()
    local rgb_theme = theme.create("rgb-example")

    -- Use RGB color objects
    theme.set_color(rgb_theme, theme.HL_KEYWORD, theme.rgb(255, 107, 107), theme.rgb(0, 0, 0), {bold = true})
    theme.set_color(rgb_theme, theme.HL_STRING, theme.rgb(78, 205, 196), theme.rgb(0, 0, 0), {})
    theme.set_color(rgb_theme, theme.HL_NUMBER, theme.rgb(255, 230, 109), theme.rgb(0, 0, 0), {})
    theme.set_color(rgb_theme, theme.HL_COMMENT, theme.rgb(149, 165, 166), theme.rgb(0, 0, 0), {italic = true})
    theme.set_color(rgb_theme, theme.HL_FUNCTION, theme.rgb(168, 230, 207), theme.rgb(0, 0, 0), {bold = true})
    theme.set_color(rgb_theme, theme.HL_TYPE, theme.rgb(52, 152, 219), theme.rgb(0, 0, 0), {})
    theme.set_color(rgb_theme, theme.HL_OPERATOR, theme.rgb(236, 240, 241), theme.rgb(0, 0, 0), {})
    theme.set_color(rgb_theme, theme.HL_VARIABLE, theme.rgb(236, 240, 241), theme.rgb(0, 0, 0), {})
    theme.set_color(rgb_theme, theme.HL_CONSTANT, theme.rgb(230, 126, 34), theme.rgb(0, 0, 0), {})
    theme.set_color(rgb_theme, theme.HL_PREPROCESSOR, theme.rgb(231, 76, 60), theme.rgb(0, 0, 0), {})
    theme.set_color(rgb_theme, theme.HL_ERROR, theme.rgb(192, 57, 43), theme.rgb(255, 255, 255), {reverse = true})

    theme.register(rgb_theme)

    editor.message("RGB theme 'rgb-example' created!")
end

-- Automatically create custom themes on load
create_custom_theme()
create_rgb_theme()

-- Export functions for use by other plugins or keybindings
return {
    list_themes = list_themes,
    set_theme = set_theme,
    get_current_theme = get_current_theme,
    create_custom_theme = create_custom_theme,
    create_rgb_theme = create_rgb_theme
}
