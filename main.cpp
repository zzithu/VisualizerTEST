#include "Visualizer.cpp"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <iostream>
#include <chrono>

/*
    This was initially desigined as a test class, now however, some
    of the preprocessing will need to be edited in order to
    have consistent usage.



    This is now deprecated, but is good for a demo. The final project will link this visualizer
    directly to an audio plugin.
*/

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_AudioStream *stream = NULL;
static Uint8 *wav_data = NULL;
static Uint32 wav_data_len = 0;


//Standard SDL Window stuff
const int WINDOW_HEIGHT = 480;
const int WINDOW_LENGTH = 640;

// Define debug macros
#ifdef DEBUG
    #define DEBUG_PRINT(msg) std::cout << "DEBUG: " << msg << std::endl;
#else
    #define DEBUG_PRINT(msg) // Empty, so it's removed in production
#endif


//Testing
int main() {
    using Complex = std::complex<float>;

    DEBUG_PRINT("Initializing");
    //Initial Setup
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    DEBUG_PRINT("Creating Window");
    /* we don't _need_ a window for audio-only things but it's good policy to have one. */
    if (!SDL_CreateWindowAndRenderer("Visualizer Test", WINDOW_LENGTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_AudioSpec wavSpec;
    Uint8* wavBuffer;
    Uint32 wavLength;


    DEBUG_PRINT("Loading file");
    //Loads wav into buffer, determines spec
    if (!SDL_LoadWAV("songs/test.wav", &wavSpec, &wavBuffer, &wavLength)) {
        std::cout << "Failed to open file: " << SDL_GetError() << std::endl;
        SDL_Delay(10000);
        SDL_Quit();
        return -1;
    }


    /*
    //Were going to ensure something is beign added here
    for (size_t i = 0; i < wavLength; ++i) { // Print first 10 bytes (or fewer)
        std::cout << static_cast<int>(wavBuffer[i]) << " ";
    }
    std::cout << std::endl;
    */
    DEBUG_PRINT("Is there a buffer? " << !wavBuffer);


    DEBUG_PRINT("Attempting to open audio device");
    //Ensure a device can listen to it?
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &wavSpec, NULL, NULL);
    if (!stream) {
        SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
        DEBUG_PRINT("ERROR Opening Audio Device");
        return -1;
    }

    //Begin visualizer incorporation
    Visualizer visualizer(32, WINDOW_LENGTH, WINDOW_HEIGHT, 44100, 4096);


     //We can find out the specific format, useful for the conversion
    DEBUG_PRINT("Determining Audio format\n");
    SDL_AudioSpec spec;
    if (!SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr)) {
        std::cerr << "Failed to get audio format!" << std::endl;
    }

    DEBUG_PRINT("Format: " << spec.format);

    DEBUG_PRINT("Attempting Conversion\n");

    std::vector<float> audioSamples;
    if (spec.format == SDL_AUDIO_F32) {
        DEBUG_PRINT("Converting to Float");
        for(size_t i = 0; i < wavLength; i++)
        {
            audioSamples.push_back(float(wavBuffer[i]));
        }

    
        // Print the first few samples
        DEBUG_PRINT("First 10 audio samples (converted to float): ");
        for (size_t i = 0; i < std::min<size_t>(1000, audioSamples.size()); ++i) {
            std::cout << audioSamples[i] << " ";
        }
        std::cout << std::endl;
    } else {
        std::cerr << "Unsupported format, need manual conversion!" << std::endl;
    }

    SDL_ResumeAudioStreamDevice(stream); 
    SDL_ResumeAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK); 
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    //App control flow
    bool exit = false;
    SDL_Event event;

    bool isInitialized = false; 
    int current;  
    int increment = 2048; // To ttest, will change in future!

    

    DEBUG_PRINT("Attempting to run program loop\n");
    while (!exit) {
        auto start = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::milliseconds(67);
        // 🎮 Handle events (Allows closing window)
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                exit = true; // Exit on window close
            }
            
        }

        DEBUG_PRINT("Inserting audio\n " << wavBuffer[0]);
        DEBUG_PRINT("Is Audio? " << wavLength);
        // 🎵 Feed more data to the audio stream
        SDL_PutAudioStreamData(stream, wavBuffer, wavLength);

        // 🛠️ FFT Processing -- NOTE: When working with FFT, pass samples, not the entire file doofus.
        DEBUG_PRINT("FFT processing\n");
        
        
        if (!isInitialized) {
            // Initialize the variable the first time
            current = 0;  
            FFT::setup(audioSamples);
            DEBUG_PRINT("Finihed Setup");
            isInitialized = true;  
        } else {
            current += increment/2;
        }

        std::vector<Complex> fftData = FFT::compute(current, increment);
        DEBUG_PRINT("Finished Computation");


        DEBUG_PRINT("Calling visualizer\n");
        visualizer.update(fftData);

        DEBUG_PRINT("Calling render\n");
        visualizer.render(renderer);    
        

        /*
            To alleviate jitter, I did some nonsense to allow waiting
        */
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> dt = end - start;
        start = end;

        // Calculate remaining time
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(dt);
        auto remainingTime = frameDuration - elapsedTime;
        int remainingTimeInt = static_cast<int>(remainingTime.count());

        if (remainingTimeInt > 0)
        {
            SDL_Delay(remainingTimeInt);  // SDL_Delay expects an integer representing milliseconds
        }

    }

 
    //Cleanup HAHAHA MEMORY LEAKS
    SDL_CloseAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK); // Close the audio device 



    SDL_Delay(10000);
    SDL_Quit();

    return 0;
}
