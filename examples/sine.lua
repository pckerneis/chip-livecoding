-- Simple sine wave example
return function(t)
    -- Generate a 440Hz sine wave
    return chip.sin(t, 440)  -- Note: using chip.sin instead of just sin
end