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
#endif

// Audio settings
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512
#define PI 3.14159265358979323846

// Audio state
AudioState audio_state = {0};
static PaStream *stream = NULL;
static int audio_initialized = 0;

// Threading
#ifdef _WIN32
static HANDLE producer_thread = NULL;
static DWORD WINAPI producer_func(LPVOID arg);
#else
#include <pthread.h>
static pthread_t producer_thread;
static void *producer_func(void *arg);
#endif

// Forward declarations
static int audio_callback(const void *input, void *output,
                         unsigned long frame_count,
                         const PaStreamCallbackTimeInfo *time_info,
                         PaStreamCallbackFlags status_flags,
                         void *user_data);

// Initialize audio system
int audio_init(void) {
    if (!audio_state.L) {
        fprintf(stderr, "Error: Lua state not initialized\n");
        return 1;
    }

    printf("Audio state: L=%p, sample_rate=%d, time=%.2f\n", 
       audio_state.L, audio_state.sample_rate, audio_state.time);

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
    // Allocate ring buffer (8 buffers worth)
    if (!audio_state.rb_data) {
        audio_state.rb_size = (unsigned int)(audio_state.buffer_size * 8);
        audio_state.rb_data = (float *)calloc(audio_state.rb_size, sizeof(float));
        audio_state.rb_read = 0;
        audio_state.rb_write = 0;
        audio_state.rb_count = 0;
        audio_state.producer_running = 0;
    }
    
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
    
    /* removed unused preferredDeviceName */
    int device = paNoDevice;

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
    
    // Mark audio as initialized
    audio_initialized = 1;
    printf("Audio initialized successfully\n");
    Pa_Sleep(100);
    return 0;
}

// Process audio (called in the main loop)
int audio_process(void) {
    // printf("Audio process called\n");
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
    Pa_Sleep(10);
    
    return 0;
}

// Clean up audio resources
void audio_cleanup(void) {
    // Stop producer first
    audio_stop_producer();
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
    // Free ring buffer
    if (audio_state.rb_data) {
        free(audio_state.rb_data);
        audio_state.rb_data = NULL;
        audio_state.rb_size = 0;
        audio_state.rb_read = audio_state.rb_write = audio_state.rb_count = 0;
    }
}

// Audio callback function
static int audio_callback(const void *input, void *output,
                         unsigned long frame_count,
                         const PaStreamCallbackTimeInfo *time_info,
                         PaStreamCallbackFlags status_flags,
                         void *user_data) {
    AudioState *state = (AudioState *)user_data;
    float *out = (float *)output;
    (void)input;
    (void)time_info;
    (void)status_flags;
    
    // Initialize output to silence
    memset(out, 0, frame_count * sizeof(float));
    
    // Safety checks (no Lua access here)
    if (!state) {
        return paContinue;
    }

    // Pull from ring buffer
    unsigned long needed = frame_count;
    unsigned long i = 0;
    while (i < needed) {
        if (state->rb_count == 0) {
            // Underrun: leave remaining zeros
            break;
        }
        float s = state->rb_data[state->rb_read];
        out[i++] = s;
        state->rb_read = (state->rb_read + 1) % state->rb_size;
        state->rb_count--;
    }
    return paContinue;
}

// Producer thread: fills ring buffer by calling Lua main(t)
#ifdef _WIN32
static DWORD WINAPI producer_func(LPVOID arg)
#else
static void *producer_func(void *arg)
#endif
{
    AudioState *state = (AudioState *)arg;
    lua_State *L = state->L;
    const double dt = 1.0 / (double)state->sample_rate;

    while (state->producer_running) {
        // Fill up to one buffer worth when space available
        while (state->producer_running && state->rb_count <= state->rb_size - (unsigned int)state->buffer_size) {
            // Call Lua per-sample for one buffer
            for (int n = 0; n < state->buffer_size; n++) {
                // Push function
                lua_getglobal(L, "main");
                if (!lua_isfunction(L, -1)) {
                    lua_pop(L, 1);
                    // push silence
                    state->rb_data[state->rb_write] = 0.0f;
                } else {
                    lua_pushnumber(L, state->time);
                    if (lua_pcall(L, 1, 1, 0) != 0) {
                        // error -> silence
                        lua_pop(L, 1);
                        state->rb_data[state->rb_write] = 0.0f;
                    } else {
                        float sample = 0.0f;
                        if (lua_isnumber(L, -1)) {
                            double v = lua_tonumber(L, -1);
                            if (v > 1.0) v = 1.0;
                            if (v < -1.0) v = -1.0;
                            sample = (float)(v * state->volume);
                        }
                        lua_pop(L, 1);
                        state->rb_data[state->rb_write] = sample;
                    }
                }
                state->rb_write = (state->rb_write + 1) % state->rb_size;
                state->rb_count++;
                state->time += dt;
            }
        }
        // Sleep briefly to yield
        Pa_Sleep(1);
    }

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

int audio_start_producer(void) {
    if (!audio_initialized || audio_state.producer_running) return 0;
    audio_state.producer_running = 1;
#ifdef _WIN32
    producer_thread = CreateThread(NULL, 0, producer_func, &audio_state, 0, NULL);
    if (producer_thread == NULL) {
        audio_state.producer_running = 0;
        return 1;
    }
#else
    if (pthread_create(&producer_thread, NULL, producer_func, &audio_state) != 0) {
        audio_state.producer_running = 0;
        return 1;
    }
#endif
    return 0;
}

void audio_stop_producer(void) {
    if (!audio_state.producer_running) return;
    audio_state.producer_running = 0;
#ifdef _WIN32
    if (producer_thread) {
        WaitForSingleObject(producer_thread, INFINITE);
        CloseHandle(producer_thread);
        producer_thread = NULL;
    }
#else
    pthread_join(producer_thread, NULL);
#endif
}
