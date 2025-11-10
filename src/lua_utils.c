#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "audio.h"

// Global time variable
extern AudioState audio_state;

// Helper function to get time value from Lua stack
static double get_time(lua_State *L, int index) {
    if (lua_isnumber(L, index)) {
        return lua_tonumber(L, index);
    }
    // If no time is provided, use the internal time
    return audio_state.time;
}

// sin(t, freq, phase)
int l_sin(lua_State *L) {
    double t = get_time(L, 1);
    double freq = luaL_checknumber(L, 2);
    double phase = luaL_optnumber(L, 3, 0.0);
    
    double value = sin(2.0 * M_PI * freq * t + phase);
    lua_pushnumber(L, value);
    return 1;
}

// saw(t, freq, phase)
int l_saw(lua_State *L) {
    double t = get_time(L, 1);
    double freq = luaL_checknumber(L, 2);
    double phase = luaL_optnumber(L, 3, 0.0);
    
    double value = 2.0 * (freq * t + phase - floor(0.5 + freq * t + phase));
    lua_pushnumber(L, value);
    return 1;
}

// sq(t, freq, phase)
int l_sq(lua_State *L) {
    double t = get_time(L, 1);
    double freq = luaL_checknumber(L, 2);
    double phase = luaL_optnumber(L, 3, 0.0);
    
    double value = (fmod(freq * t + phase, 1.0) < 0.5) ? 1.0 : -1.0;
    lua_pushnumber(L, value);
    return 1;
}

// tri(t, freq, phase)
int l_tri(lua_State *L) {
    double t = get_time(L, 1);
    double freq = luaL_checknumber(L, 2);
    double phase = luaL_optnumber(L, 3, 0.0);
    
    double x = fmod(freq * t + phase, 1.0);
    double value = 1.0 - 4.0 * fabs(round(x - 0.25) - (x - 0.25));
    lua_pushnumber(L, value);
    return 1;
}

// spl(path, t)
int l_spl(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    double t = luaL_checknumber(L, 2);
    
    // For now, return 0 as we'll implement sample loading later
    lua_pushnumber(L, 0.0);
    return 1;
}

// rnd() or rnd(max) or rnd(min, max)
int l_rnd(lua_State *L) {
    if (lua_gettop(L) == 0) {
        // rnd()
        lua_pushnumber(L, (double)rand() / RAND_MAX);
    } else if (lua_gettop(L) == 1) {
        // rnd(max)
        double max = luaL_checknumber(L, 1);
        lua_pushnumber(L, ((double)rand() / RAND_MAX) * max);
    } else {
        // rnd(min, max)
        double min = luaL_checknumber(L, 1);
        double max = luaL_checknumber(L, 2);
        lua_pushnumber(L, min + ((double)rand() / RAND_MAX) * (max - min));
    }
    return 1;
}

// rndf() or rndf(max) or rndf(min, max)
int l_rndf(lua_State *L) {
    return l_rnd(L); // Same as rnd for now
}

// rndi(max) or rndi(min, max)
int l_rndi(lua_State *L) {
    if (lua_gettop(L) == 1) {
        // rndi(max)
        int max = luaL_checkinteger(L, 1);
        lua_pushinteger(L, rand() % (max + 1));
    } else {
        // rndi(min, max)
        int min = luaL_checkinteger(L, 1);
        int max = luaL_checkinteger(L, 2);
        lua_pushinteger(L, min + (rand() % (max - min + 1)));
    }
    return 1;
}

// Register all functions in the Lua state
static const luaL_Reg chip_lib[] = {
    {"sin", l_sin},
    {"saw", l_saw},
    {"sq", l_sq},
    {"tri", l_tri},
    {"spl", l_spl},
    {"rnd", l_rnd},
    {"rndf", l_rndf},
    {"rndi", l_rndi},
    {NULL, NULL}
};

// Open the library
int luaopen_audio(lua_State *L) {
    luaL_newlib(L, chip_lib);
    
    // Store audio state in the registry
    lua_pushlightuserdata(L, &audio_state);
    lua_setfield(L, LUA_REGISTRYINDEX, "chip.audio_state");
    
    return 1;
}
