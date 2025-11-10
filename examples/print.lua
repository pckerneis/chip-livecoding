-- Simple print example
function main(t)
    -- Print only once to avoid flooding the console
    if t < 0.1 then
        chip.print("Hello from Lua! Time: " .. t)
    end
    return 0  -- Return 0 as a silent audio sample
end

-- Return the main function
return main