#pragma once
#include <SDL3/SDL.h>
#include "FFT.cpp"
#include <iostream>

class Visualizer {
private:
    std::vector<std::complex<float>> freqData;  // Stores (frequency, amplitude) pairs
    std::vector<SDL_FRect> soundBars; // SDL rectangles for visualization
    std::vector<float> logBins;


    int numBins;        // Number of frequency bins
    int screenWidth;    // Screen width
    int screenHeight;   // Screen height
    int sampleRate;     // Audio sample rate (e.g., 44100 Hz)
    int fftSize;        // FFT window size (e.g., 1024)
    float binWidth;
    int BAR_HEIGHT_MAX;
   

public:
    using Complex = std::complex<float>;

    // Constructor & Destructor
    Visualizer(int bins, int width, int height, int sampleRate, int fftSize);
    ~Visualizer();

    // Creates necessary rectangles for visualization
    bool createScene();

    // Updates the sound bars based on frequency and amplitude data
    int update(const std::vector<Complex>& input);

    // Renders the bars using SDL
    void render(SDL_Renderer* renderer);

private:
    // Calculates the total amplitude in a given bin
    float sumAmplitude(int binIndex);

    //Responsible for estabilshing both frequency bins and pixels for soundbars
    void generateLogBins();

};
