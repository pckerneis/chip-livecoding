#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

// Compatibility for Lua 5.1
#if LUA_VERSION_NUM == 501
#ifndef luaL_newlib
#define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
#endif
#endif
#include "audio.h"

// Windows-specific includes
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#define WIN32_LEAN_AND_MEAN
#else
#include <signal.h>
#include <unistd.h>
#endif

static volatile bool running = true;

// Platform-specific signal/control handler
#ifdef _WIN32
static BOOL WINAPI console_handler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        running = false;
        return TRUE;
    }
    return FALSE;
}
#else
void handle_signal(int sig) {
    running = false;
}
#endif

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <script.lua>\n", argv[0]);
        return 1;
    }

    // Set up signal/control handlers
#ifdef _WIN32
    if (!SetConsoleCtrlHandler(console_handler, TRUE)) {
        fprintf(stderr, "Failed to set control handler\n");
        return 1;
    }
#else
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
#endif

    // Initialize Lua state
    lua_State *L = luaL_newstate();
    if (!L) {
        fprintf(stderr, "Failed to initialize Lua\n");
        return 1;
    }
    luaL_openlibs(L);

    audio_state.L = L;

    // Initialize audio
    // if (audio_init() != 0) {
    //     fprintf(stderr, "Failed to initialize audio\n");
    //     lua_close(L);
    //     return 1;
    // }

    // Register audio module
    printf("1. About to open audio module\n");
    
    // Check stack before luaL_register
    printf("Stack before luaL_register (top=%d):\n", lua_gettop(L));
    for (int i = 1; i <= lua_gettop(L); i++) {
        printf("  %d: %s\n", i, lua_typename(L, lua_type(L, i)));
    }
    
    // Try creating the module using lua_newtable instead
    printf("Creating chip table...\n");
    lua_newtable(L);  // Create new table
    lua_setglobal(L, "chip");  // Set as global 'chip'
    
    // Now get the table back and call luaopen_audio
    printf("Getting chip table...\n");
    lua_getglobal(L, "chip");
    if (!lua_istable(L, -1)) {
        printf("ERROR: Failed to get chip table\n");
        return 1;
    }
    
    printf("Calling luaopen_audio...\n");
    luaopen_audio(L);  // This will populate the 'chip' table with functions
    
    printf("3. Audio module loaded successfully\n");

    // Load and run the script
    printf("About to load script\n");
    if (luaL_dofile(L, argv[1]) != 0) {
        fprintf(stderr, "Error loading script: %s\n", lua_tostring(L, -1));
        lua_close(L);
        audio_cleanup();
        return 1;
    }

    // Debug
    luaL_dostring(L, "function main(t) return math.sin(t) end");

    printf("Lua script loaded successfully\n");

    printf("Chip-Livecoding running. Press Ctrl+C to exit.\n");

    printf("Entering main loop...\n");
    // Main loop
    while (running) {
        // Process audio
        if (audio_process() != 0) {
            fprintf(stderr, "Audio processing error\n");
            break;
        }

        // Check for exit condition (Windows)
#ifdef _WIN32
        if (_kbhit() && _getch() == 3) {  // Ctrl+C
            running = false;
        }
#endif
    }

    // Cleanup
    audio_cleanup();
    lua_close(L);

    return 0;
}
