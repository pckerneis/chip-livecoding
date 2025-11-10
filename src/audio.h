#ifndef AUDIO_H
#define AUDIO_H

#include <lua.h>

typedef struct AudioState {
    lua_State *L;
    double time;
    int sample_rate;
    int buffer_size;
    float volume;
    // Ring buffer for pre-rendered audio (mono)
    float *rb_data;
    unsigned int rb_size;     // total samples capacity
    volatile unsigned int rb_read;  // read index
    volatile unsigned int rb_write; // write index
    volatile unsigned int rb_count; // number of samples available
    volatile int producer_running;
    // Live reload support
    char script_path[256];
    long script_mtime;
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

// Producer control
int audio_start_producer(void);
void audio_stop_producer(void);

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
