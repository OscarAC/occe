-- Window Layout Algorithms Plugin
-- Provides custom layout algorithms for window arrangement

-- Golden ratio layout - gives more space to the focused window
window.register_layout("golden", function(windows, total_w, total_h)
    local phi = 1.618  -- Golden ratio
    local count = #windows

    if count == 0 then
        return {}
    elseif count == 1 then
        return {{
            id = windows[1],
            x = 0,
            y = 0,
            w = total_w,
            h = total_h
        }}
    elseif count == 2 then
        -- Split vertically using golden ratio
        local main_w = math.floor(total_w / phi)
        return {
            {id = windows[1], x = 0, y = 0, w = main_w, h = total_h},
            {id = windows[2], x = main_w, y = 0, w = total_w - main_w, h = total_h}
        }
    else
        -- Main window takes golden ratio, others stack vertically
        local main_w = math.floor(total_w / phi)
        local side_w = total_w - main_w
        local side_h = math.floor(total_h / (count - 1))

        local layout = {{id = windows[1], x = 0, y = 0, w = main_w, h = total_h}}

        for i = 2, count do
            table.insert(layout, {
                id = windows[i],
                x = main_w,
                y = (i - 2) * side_h,
                w = side_w,
                h = side_h
            })
        end

        return layout
    end
end)

-- Grid layout - arrange windows in a grid
window.register_layout("grid", function(windows, total_w, total_h)
    local count = #windows

    if count == 0 then
        return {}
    elseif count == 1 then
        return {{id = windows[1], x = 0, y = 0, w = total_w, h = total_h}}
    end

    -- Calculate grid dimensions
    local cols = math.ceil(math.sqrt(count))
    local rows = math.ceil(count / cols)

    local cell_w = math.floor(total_w / cols)
    local cell_h = math.floor(total_h / rows)

    local layout = {}
    for i, win_id in ipairs(windows) do
        local row = math.floor((i - 1) / cols)
        local col = (i - 1) % cols

        table.insert(layout, {
            id = win_id,
            x = col * cell_w,
            y = row * cell_h,
            w = cell_w,
            h = cell_h
        })
    end

    return layout
end)

-- Column layout - all windows in vertical columns with equal width
window.register_layout("columns", function(windows, total_w, total_h)
    local count = #windows

    if count == 0 then
        return {}
    end

    local col_w = math.floor(total_w / count)
    local layout = {}

    for i, win_id in ipairs(windows) do
        table.insert(layout, {
            id = win_id,
            x = (i - 1) * col_w,
            y = 0,
            w = col_w,
            h = total_h
        })
    end

    return layout
end)

-- Stack layout - all windows stacked horizontally with equal height
window.register_layout("stack", function(windows, total_w, total_h)
    local count = #windows

    if count == 0 then
        return {}
    end

    local row_h = math.floor(total_h / count)
    local layout = {}

    for i, win_id in ipairs(windows) do
        table.insert(layout, {
            id = win_id,
            x = 0,
            y = (i - 1) * row_h,
            w = total_w,
            h = row_h
        })
    end

    return layout
end)

-- Helper function to apply a layout by name
function apply_window_layout(layout_name)
    local count = window.get_count()
    if count <= 1 then
        editor.message("Need at least 2 windows for layout")
        return
    end

    window.apply_layout(layout_name)
    editor.message("Applied '" .. layout_name .. "' layout (" .. count .. " windows)")
end

editor.message("Layout plugin loaded! Available: golden, grid, columns, stack")
