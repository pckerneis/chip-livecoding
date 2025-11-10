#ifndef AUDIO_H
#define AUDIO_H

#include <lua.h>

typedef struct AudioState {
    lua_State *L;
    double time;
    int sample_rate;
    int buffer_size;
    float volume;
} AudioState;

extern AudioState audio_state;

// Initialize audio system
int audio_init(void);

// Process audio (called in the main loop)
int audio_process(void);

// Clean up audio resources
void audio_cleanup(void);

// Lua API functions
int luaopen_audio(lua_State *L);

// Audio generation functions exposed to Lua
int l_sin(lua_State *L);
int l_saw(lua_State *L);
int l_sq(lua_State *L);
int l_tri(lua_State *L);
int l_spl(lua_State *L);
int l_rnd(lua_State *L);
int l_rndf(lua_State *L);
int l_rndi(lua_State *L);

#endif // AUDIO_H
