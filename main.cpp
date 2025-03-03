//This is primarily for testings

//Visualizer
#include "Visualizer.cpp"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <iostream>

//FFT


static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_AudioStream *stream = NULL;
static Uint8 *wav_data = NULL;
static Uint32 wav_data_len = 0;


//Standard SDL Window stuff
const int WINDOW_HEIGHT = 480;
const int WINDOW_LENGTH = 640;

//Testing
int main() {
    using Complex = std::complex<float>;

    printf("Initializing");
    //Initial Setup
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    printf("Creating Window");
    /* we don't _need_ a window for audio-only things but it's good policy to have one. */
    if (!SDL_CreateWindowAndRenderer("Visualizer Test", WINDOW_LENGTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_AudioSpec wavSpec;
    Uint8* wavBuffer;
    Uint32 wavLength;


    printf("Loading file");
    //Loads wav into buffer, determines spec
    if (!SDL_LoadWAV("songs/Feet.wav", &wavSpec, &wavBuffer, &wavLength)) {
        std::cout << "Failed to open file: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    printf("Attempting to open audio device");
    //Ensure a device can listen to it?
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &wavSpec, NULL, NULL);
    if (!stream) {
        SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
        return -1;
    }

    //Begin visualizer incorporation
    Visualizer visualizer(64, WINDOW_LENGTH, WINDOW_HEIGHT, 44100, 1024);


     //We can find out the specific format, useful for the conversion
     printf("Determining Audio format\n");
     SDL_AudioSpec spec;
    if (!SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr)) {
        std::cerr << "Failed to get audio format!" << std::endl;
    }

    // 🎵 Convert Uint8* → std::vector<float>
    printf("Attempting Conversion\n");
    std::vector<float> audioSamples;
    if (spec.format == SDL_AUDIO_F32) {
        float* samples = reinterpret_cast<float*>(wavBuffer);
        Uint32 sampleCount = wavLength / sizeof(float);
        audioSamples.assign(samples, samples + sampleCount);
    } else {
        std::cerr << "Unsupported format, need manual conversion!" << std::endl;
    }

    SDL_ResumeAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    //App control flow
    bool exit = false;
    SDL_Event event;

    printf("Attempting to run program loop\n");
    while (!exit) {
    
        // 🎮 Handle events (Allows closing window)
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                exit = true; // Exit on window close
            }
            
        }

        printf("Inserting audio\n");
        // 🎵 Feed more data to the audio stream
        SDL_PutAudioStreamData(stream, wavBuffer, wavLength);

        // 🛠️ FFT Processing
        printf("FFT processing\n");
        std::vector<Complex> fftData = FFT::compute(audioSamples);
        printf("Calling visualizer\n");
        visualizer.update(fftData);

        printf("Calling render\n");
        visualizer.render(renderer);    
        
        
    }

 
    //Cleanup HAHAHA MEMORY LEAKS
    SDL_CloseAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK); // Close the audio device 



    SDL_Delay(10000);
    SDL_Quit();

    return 0;
}
