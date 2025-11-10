#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <portaudio.h>
#include <sndfile.h>
#include <time.h>
#include <string.h>
#include "audio.h"

// Windows-specific includes
#ifdef _WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
#else
#include <unistd.h>
#define sleep_ms(ms) usleep((ms) * 1000)
#endif

// Audio settings
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512
#define PI 3.14159265358979323846

// Audio state
AudioState audio_state = {0};
static PaStream *stream = NULL;
static int audio_initialized = 0;

// Forward declarations
static int audio_callback(const void *input, void *output,
                         unsigned long frame_count,
                         const PaStreamCallbackTimeInfo *time_info,
                         PaStreamCallbackFlags status_flags,
                         void *user_data);

// Initialize audio system
int audio_init(void) {
    PaError err;
    
    if (audio_initialized) {
        return 0; // Already initialized
    }
    
    // Initialize random number generator
    srand((unsigned int)time(NULL));
    
    // Initialize PortAudio
    err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }
    
    // Set up audio state
    audio_state.sample_rate = SAMPLE_RATE;
    audio_state.buffer_size = FRAMES_PER_BUFFER;
    audio_state.time = 0.0;
    audio_state.volume = 0.5f; // Default volume
    
    // List available devices
    printf("Available audio devices:\n");
    int numDevices = Pa_GetDeviceCount();
    for (int i = 0; i < numDevices; i++) {
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
        printf("%d: %s (in: %d, out: %d)\n", 
               i, deviceInfo->name, 
               deviceInfo->maxInputChannels, 
               deviceInfo->maxOutputChannels);
    }
    
    const char *preferredDeviceName = "default";
    int device = paNoDevice;

    // In audio_init, after listing devices, add this:
    printf("\nTrying to find a working audio device...\n");
    for (int i = 0; i < numDevices; i++) {
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
        if (!deviceInfo) continue;  // Skip if device info is null
        
        printf("Trying device %d: %s (in: %d, out: %d)\n", 
            i, deviceInfo->name, 
            deviceInfo->maxInputChannels, 
            deviceInfo->maxOutputChannels);
        
        if (deviceInfo->maxOutputChannels > 0) {
            PaStreamParameters outputParameters = {0};
            outputParameters.device = i;
            outputParameters.channelCount = 1;
            outputParameters.sampleFormat = paFloat32;
            outputParameters.suggestedLatency = deviceInfo->defaultLowOutputLatency;
            outputParameters.hostApiSpecificStreamInfo = NULL;
            
            printf("  Trying to open with %dHz, %d frames/buffer...\n", 
                audio_state.sample_rate, audio_state.buffer_size);
            
            PaError err = Pa_OpenStream(
                &stream,
                NULL, // No input
                &outputParameters,
                audio_state.sample_rate,
                audio_state.buffer_size,
                paNoFlag,  // Try without any special flags first
                audio_callback,
                &audio_state);
                
            if (err == paNoError) {
                printf("Successfully opened device %d: %s\n", i, deviceInfo->name);
                device = i;
                break;
            } else {
                printf("  Failed to open device: %s\n", Pa_GetErrorText(err));
            }
        }
    }
    
    if (device == paNoDevice) {
        fprintf(stderr, "Error: No suitable output device found\n");
        Pa_Terminate();
        return 1;
    }
    
    // Start the stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "Error starting audio stream: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    Pa_Sleep(100);
    
    audio_initialized = 1;
    return 0;
}

// Process audio (called in the main loop)
int audio_process(void) {
    printf("Audio process called\n");
    if (!audio_initialized) {
        printf("Audio not initialized\n");
        return 1;
    }
    
    // Check if stream is active
    PaError err = Pa_IsStreamActive(stream);
    if (err < 0) {
        fprintf(stderr, "PortAudio stream error: %s\n", Pa_GetErrorText(err));
        return 1;
    }
    
    // Small sleep to prevent busy-waiting
    sleep_ms(10);
    
    return 0;
}

// Clean up audio resources
void audio_cleanup(void) {
    if (!audio_initialized) {
        return; // Already cleaned up
    }
    
    if (stream) {
        PaError err;
        
        // Stop the stream
        err = Pa_StopStream(stream);
        if (err != paNoError) {
            fprintf(stderr, "Warning: Failed to stop stream: %s\n", Pa_GetErrorText(err));
        }
        
        // Close the stream
        err = Pa_CloseStream(stream);
        if (err != paNoError) {
            fprintf(stderr, "Warning: Failed to close stream: %s\n", Pa_GetErrorText(err));
        }
        
        stream = NULL;
    }
    
    // Terminate PortAudio
    PaError err = Pa_Terminate();
    if (err != paNoError) {
        fprintf(stderr, "Warning: Failed to terminate PortAudio: %s\n", Pa_GetErrorText(err));
    }
    
    audio_initialized = 0;
}

// Audio callback function
static int audio_callback(const void *input, void *output,
                         unsigned long frame_count,
                         const PaStreamCallbackTimeInfo *time_info,
                         PaStreamCallbackFlags status_flags,
                         void *user_data) {
    printf("Audio callback called\n");
    AudioState *state = (AudioState *)user_data;
    if (!state || !state->L) {
        printf("Error: Invalid audio state or Lua state\n");
        return paComplete;
    }

    AudioState *state = (AudioState *)user_data;
    float *out = (float *)output;
    
    // Initialize output to silence
    memset(out, 0, frame_count * sizeof(float));
    
    // Get the Lua function from the stack
    lua_State *L = state->L;
    if (!L) {
        return paContinue; // No Lua state available
    }
    
    lua_getglobal(L, "main");
    
    if (!lua_isfunction(L, -1)) {
        // No main function defined, output silence
        lua_pop(L, 1); // Pop the non-function value
        return paContinue;
    }
    
    // Generate audio samples
    for (unsigned long i = 0; i < frame_count; i++) {
        // Push the time argument
        lua_pushnumber(L, state->time);
        
        // Call the Lua function with 1 argument and 1 result
        if (lua_pcall(L, 1, 1, 0) != 0) {
            // Error in Lua code, output silence
            const char *err_msg = lua_tostring(L, -1);
            if (err_msg) {
                fprintf(stderr, "Lua error: %s\n", err_msg);
            } else {
                fprintf(stderr, "Unknown Lua error\n");
            }
            lua_pop(L, 1); // Pop error message
            out[i] = 0.0f;
        } else {
            // Get the result from the Lua function
            if (lua_isnumber(L, -1)) {
                double sample = lua_tonumber(L, -1);
                // Apply volume and clamp to [-1.0, 1.0]
                sample = fmax(-1.0, fmin(1.0, sample)) * state->volume;
                out[i] = (float)sample;
            }
            lua_pop(L, 1); // Pop the result
        }
        
        // Update time
        state->time += 1.0 / state->sample_rate;
    }
    
    return paContinue;
}
