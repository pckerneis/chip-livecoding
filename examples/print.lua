-- Simple print example
local has_printed = false

local function main(t)
    -- Print only once to avoid flooding the console
    if not has_printed then
        chip.print("Hello from Lua! Time: " .. t)
        has_printed = true
    end
    return 0  -- Return 0 as a silent audio sample
end

-- Return the main function
return main