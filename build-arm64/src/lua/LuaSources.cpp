#include "lua/LuaSources.h"

namespace Surge::LuaSources {

const std::string formula_prelude = R"EOF(-- This document is loaded in each Surge XT session and provides a set of built-in helpers
-- we've found handy when writing modulators. Consider it as a library of functions.
-- For each official update of Surge XT we will freeze the state of the prelude as stable
-- and not break included functions after that.
--
-- If you have ideas for other useful functions that could be added here, by all means
-- contact us over GitHub or Discord and let us know!


local surge = {}
local mod = {}


--- MATH FUNCTIONS ---


-- parity function returns 0 for even numbers and 1 for odd numbers
function math.parity(x)
    return (x % 2 == 1 and 1) or 0
end

-- signum function returns -1 for negative numbers, 0 for zero, 1 for positive numbers
function math.sgn(x)
    return (x > 0 and 1) or (x < 0 and -1) or 0
end

-- sign function returns -1 for negative numbers and 1 for positive numbers or zero
function math.sign(x)
    return (x < 0 and -1) or 1
end

-- linearly interpolates value from in range to out range
function math.rescale(value, in_min, in_max, out_min, out_max)
    return (((value - in_min) * (out_max - out_min)) / (in_max - in_min)) + out_min
end

-- returns the norm of the two components (hypotenuse)
function math.norm(a, b)
    return math.sqrt(a ^ 2 + b ^ 2)
end

-- returns the absolute range between the two numbers
function math.range(a, b)
    return math.abs(a - b)
end

-- returns the frequency of the given MIDI note number under A440 equal temperament
-- ref is optional and allows specifying a different center frequency for
-- MIDI note 69 (middle A)
function math.note_to_freq(note, ref)
    local default = 440
    ref = ref or default
    return 2^((note - 69) / 12) * ref
end

-- returns the fractional MIDI note number matching given frequency under A440
-- equal temperament
-- ref is optional and allows specifying a different center frequency for
-- MIDI note 69 (middle A)
function math.freq_to_note(freq, ref)
    local default = 440
    ref = ref or default
    return 12 * math.log((freq / ref), 2) + 69
end

-- returns greatest common denominator between a and b
-- use with integers only!
function math.gcd(a, b)
    local x = a
    local y = b
    local t

    while y ~= 0 do
        t = y
        y = x % y
        x = t
    end

    return x
end

-- returns least common multiple between a and b
-- use with integers only!
function math.lcm(a, b)
    local t = a

    while t % b ~= 0 do
        t = t + a
    end

    return t
end


--- BUILT-IN MODULATORS ---


-- Clock Divider --

mod.ClockDivider =
{
    numerator = 1,
    denominator = 1,
    prioribeat = -1,
    newbeat = false,
    intphase = 0, -- increase from 0 up to n
    ibeat = 0,    -- wraps with denominator
    phase = 0
}

mod.ClockDivider.new = function(self, o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

mod.ClockDivider.tick = function(self, intphase, phase)
    local beat = (intphase + phase) * self.numerator / self.denominator
    local ibeat = math.floor(beat)

    self.intphase = ibeat
    self.ibeat = ibeat % self.numerator
    self.phase = beat - ibeat
    self.newbeat = false

    if (ibeat ~= self.prioribeat) then
        self.newbeat = true
    end

    self.prioribeat = ibeat
end

-- AHD Envelope --

mod.AHDEnvelope =
{
    a = 0.1,
    h = 0.1,
    d = 0.7
}

mod.AHDEnvelope.new = function(self, o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

mod.AHDEnvelope.at = function(self, phase)
    if (phase <= 0) then
        return 0.0
    elseif (phase < self.a) then
        return phase / self.a
    elseif (phase < self.a + self.h) then
        return 1.0
    elseif (phase < self.a + self.h + self.d) then
        return 1.0 - (phase - (self.a + self.h)) / self.d
    else
        return 0.0
    end
end

-- Slew Limiter --

mod.Slew =
{
    prior = 0,
    block_factor = 1,
    block_size = 0,
    samplerate = 0,
    calculate = true,
    is_first = true
}

mod.Slew.new = function(self, o)
    o = o or {}
    o.block_factor = 0
    setmetatable(o, self)
    self.__index = self

    if (o.samplerate == 0 or o.block_size == 0) then
        o.calculate = false
    end

    o.block_factor = (o.samplerate / 48000) / (o.block_size / 32)

    return o
end

mod.Slew.run = function(self, input, up_rate, down_rate)

    if (not self.calculate) then
        return 0
    end

    if (self.is_first) then
        self.prior = input
        self.is_first = false
    end

    if (up_rate == nil) then
        up_rate = 0.2
    end
    
    if (up_rate < 0) then
        up_rate = 0
    end
    
    local up_limit = 1 / (1 + (10000 * self.block_factor) * up_rate^3)
    local down_limit = 1
    
    if (down_rate == nil) then
        down_limit = up_limit
    else
        if (down_rate < 0) then
            down_rate = 0
        end

        down_limit = 1 / (1 + (10000 * self.block_factor) * down_rate^3)
    end
    
    local delta = input - self.prior

    if (delta > up_limit) then
        delta = up_limit
    end

    if (delta < -down_limit) then
        delta = -down_limit
    end

    self.prior = self.prior + delta
    
    return self.prior
end


--- END ---


surge.mod = mod

return surge
)EOF";

const std::string formula_prelude_test = R"EOF(-- surge = loadfile( "src/lua/surge_prelude.lua")();
-- loadfile("src/lua/surge_prelude_test.lua")();
-- print(test())

function test()

    a = surge.mod.ClockDivider:new()
    if (a.numerator ~= 1 and a.denominator ~= 1) then
        error("Incorrect constructor of Clock a", 2)
    end

    b = surge.mod.ClockDivider:new { numerator = 3 }
    if (b.numerator ~= 3 and b.denominator ~= 1) then
        error("Incorrect constructor of Clock b", 2)
    end

    c = surge.mod.ClockDivider:new { numerator = 5, denominator = 2 }
    if (c.numerator ~= 5 and c.denominator ~= 2) then
        error("Incorrect constructor of Clock c", 2)
    end

    dphase = 1.5 / 13
    iphase = 0
    phase = 0.0
    tick = {}
    tick["a"] = 0
    tick["b"] = 0
    tick["c"] = 0
    while (iphase < 3) do
        a:tick(iphase, phase)
        b:tick(iphase, phase)
        c:tick(iphase, phase)

        tick["a"] = tick["a"] + (a.newbeat and 1 or 0)
        tick["b"] = tick["b"] + (b.newbeat and 1 or 0)
        tick["c"] = tick["c"] + (c.newbeat and 1 or 0)

        phase = phase + dphase
        if (phase > 1) then
            phase = phase - 1
            iphase = iphase + 1
        end
    end

    if (tick["a"] ~= 3 and tick["b"] ~= 9 and tick["c"] ~= 8) then
        error("Tick calculation off", 2)
    end

    en = surge.mod.AHDEnvelope:new { a = 0.2, h = 0.3, d = 0.1 }
    if (en:at(0) ~= 0) then
        error("Bad Start Envelope", 2)
    end
    if (math.abs(en:at(0.05) - 0.25) > 0.001) then
        error("Bad Clumb", 2);
    end
    if (en:at(0.25) ~= 1) then
        error("Bad Hold", 2);
    end
    if (math.abs(en:at(0.525) - 0.75) > 0.001) then
        error("Bad Drop", 2)
    end

    return 1
end

)EOF";

const std::string wtse_prelude = R"EOF(-- This document is loaded in each Surge XT session and provides a set of built-in helpers
-- we've found handy when generating wavetables. Consider it as a library of functions.
-- For each official update of Surge XT we will freeze the state of the prelude as stable
-- and not break included functions after that.
--
-- If you have ideas for other useful functions that could be added here, by all means
-- contact us over GitHub or Discord and let us know!


local surge = {}
local mod = {}


--- MATH FUNCTIONS ---


-- parity function returns 0 for even numbers and 1 for odd numbers
function math.parity(x)
    return (x % 2 == 1 and 1) or 0
end

-- signum function returns -1 for negative numbers, 0 for zero, 1 for positive numbers
function math.sgn(x)
    return (x > 0 and 1) or (x < 0 and -1) or 0
end

-- sign function returns -1 for negative numbers and 1 for positive numbers or zero
function math.sign(x)
    return (x < 0 and -1) or 1
end

-- linearly interpolates value from in range to out range
function math.rescale(value, in_min, in_max, out_min, out_max)
    return (((value - in_min) * (out_max - out_min)) / (in_max - in_min)) + out_min
end

-- returns the norm of the two components (hypotenuse)
function math.norm(a, b)
    return math.sqrt(a ^ 2 + b ^ 2)
end

-- returns the absolute range between the two numbers
function math.range(a, b)
    return math.abs(a - b)
end

-- returns greatest common denominator between a and b
-- use with integers only!
function math.gcd(a, b)
    local x = a
    local y = b
    local t

    while y ~= 0 do
        t = y
        y = x % y
        x = t
    end

    return x
end

-- returns least common multiple between a and b
-- use with integers only!
function math.lcm(a, b)
    local t = a

    while t % b ~= 0 do
        t = t + a
    end

    return t
end


--- WTSE SPECIFIC MATH FUNCTIONS ---


-- returns a table with the cumulative product of the elements in the input table
function math.cumprod(t)
    local o = {}
    o[1] = t[1]
    for i = 2, #t do
        o[i] = o[i - 1] * t[i]
    end
    return o
end

-- returns a table with the cumulative sum of the elements in the input table
function math.cumsum(t)
    local o = {}
    o[1] = t[1]
    for i = 2, #t do
        o[i] = o[i - 1] + t[i]
    end
    return o
end

-- returns a table containing num_points linearly spaced numbers from start_point to end_point
function math.linspace(start_point, end_point, num_points)
    if num_points < 2 then
        return {start_point}
    end
    local t = {}
    local step = (end_point - start_point) / (num_points - 1)
    for i = 1, num_points do
        t[i] = start_point + (i - 1) * step
    end
    return t
end

-- returns a table containing num_points logarithmically spaced numbers from 10^start_point to 10^end_point
function math.logspace(start_point, end_point, num_points)
    if num_points < 2 then
        return {start_point}
    end
    local t = {}
    local step = (end_point - start_point) / (num_points - 1)
    for i = 1, num_points do
        local exponent = start_point + (i - 1) * step
        t[i] = 10 ^ exponent
    end
    return t
end

-- returns a table of length n, or a multidimensional table with {n, n, ..} dimensions all initialized with zeros
function math.zeros(dimensions)
    if type(dimensions) == "number" then
        dimensions = {dimensions}
    elseif type(dimensions) ~= "table" or #dimensions == 0 then
        return {0}
    end
    local function create_array(dimensions, depth)
        local size = dimensions[depth]
        local t = {}
        for i = 1, size do
            if depth < #dimensions then
                t[i] = create_array(dimensions, depth + 1)
            else
                t[i] = 0
            end
        end
        return t
    end
    return create_array(dimensions, 1)
end

-- returns a table or multidimensional table with every numerical value in the input table offset by x
function math.offset(t, x)
    local o = {}
    for k, v in pairs(t) do
        if type(v) == "table" then
            o[k] = math.offset(v, x)
        elseif type(v) == "number" then
            o[k] = v + x
        else
            o[k] = v
        end
    end
    return o
end

-- returns the maximum absolute value found in the input table
function math.max_abs(t)
    local o = 0
    for i = 1, #t do
            local a = math.abs(t[i])
            if a > o then o = a end
    end
    return o
end

-- deprecated: use math.max_abs() instead
function math.maxAbsFromTable(t)
    local o = 0
    for i = 1, #t do
            local a = math.abs(t[i])
            if a > o then o = a end
    end
    return o
end

-- returns the normalized sinc function for a table of input values
function math.sinc(t)
    local o = {}
    for i, x in ipairs(t) do
        if x == 0 then
            o[i] = 1
        else
            o[i] = math.sin(math.pi * x) / (math.pi * x)
        end
    end
    return o
end


--- BUILT-IN MODULATORS ---


-- returns a table or two dimensional table with values from the input table,
-- peak-normalized such that the maximum absolute value equals 1 (default) or the specified norm_factor
function mod.normalize_peaks(t, norm_factor)
    norm_factor = norm_factor or 1
    local max_val = 0
    local o = {}
    if type(t[1]) == "table" then
        for _, frame in ipairs(t) do
            max_val = math.max(max_val, math.max_abs(frame))
        end
    else
        max_val = math.max_abs(t)
    end
    if max_val > 0 then
        local scale_factor = norm_factor / max_val
        if type(t[1]) == "table" then
            for i, frame in ipairs(t) do
                o[i] = {}
                for j, value in ipairs(frame) do
                    o[i][j] = value * scale_factor
                end
            end
        else
            for i, value in ipairs(t) do
                o[i] = value * scale_factor
            end
        end
    else
        if type(t[1]) == "table" then
            o = math.zeros({#t, #t[1]})
        else
            o = math.zeros(#t)
        end
    end
    return o
end


--- END ---


surge.mod = mod

return surge

)EOF";

} // namespace Surge::LuaSources
