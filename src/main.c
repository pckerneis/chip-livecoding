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
    (void)sig;
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
    printf("Initializing audio...\n");
    if (audio_init() != 0) {
        fprintf(stderr, "Failed to initialize audio\n");
        lua_close(L);
        return 1;
    }

    // Create and register the chip module
    printf("1. Setting up audio module...\n");
    lua_newtable(L);  // Create the chip table
    lua_setglobal(L, "chip");  // Set as global 'chip'
    
    // Register audio functions
    lua_getglobal(L, "chip");
    luaopen_audio(L);  // This will populate the 'chip' table with functions
    lua_pop(L, 1);  // Pop the chip table
    
    printf("2. Loading script: %s\n", argv[1]);
    if (luaL_dofile(L, argv[1]) != 0) {
        fprintf(stderr, "Error loading script: %s\n", lua_tostring(L, -1));
        lua_close(L);
        audio_cleanup();
        return 1;
    }
    
    // The script should return a function, which we'll store as 'main'
    if (!lua_isfunction(L, -1)) {
        fprintf(stderr, "Script must return a function\n");
        lua_close(L);
        audio_cleanup();
        return 1;
    }
    
    // Store the returned function as 'main' in the global table
    lua_setglobal(L, "main");
    printf("3. Script loaded successfully\n");

    // Save script path for live reload
    snprintf(audio_state.script_path, sizeof(audio_state.script_path), "%s", argv[1]);

    // Start producer thread to pre-render audio into the ring buffer
    if (audio_start_producer() != 0) {
        fprintf(stderr, "Failed to start audio producer\n");
        audio_cleanup();
        lua_close(L);
        return 1;
    }

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
