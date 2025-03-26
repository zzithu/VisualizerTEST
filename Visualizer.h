#pragma once
#include "FFT.cpp"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <iostream>
#include <chrono>

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
    int style;
    float binWidth;
    float BAR_HEIGHT_MAX;
   

public:
    using Complex = std::complex<float>;

    // Constructor & Destructor
    Visualizer(int bins, int width, int height, int sampleRate, int fftSize,int style);
    ~Visualizer();

    // Creates necessary rectangles for visualization
    bool createScene();

    // Updates the sound bars based on frequency and amplitude data
    int update(const std::vector<Complex>& input);

    // Renders the bars using SDL
    void render(SDL_Renderer* renderer);

    //Added to reduce required code
    bool createSDLComponents();

private:
    // Calculates the total amplitude in a given bin
    float sumAmplitude(int binIndex);

    //Responsible for estabilshing both frequency bins and pixels for soundbars
    void generateLogBins();

};
