#include "Visualizer.h"


//Its a stylistic thing more than anything practical
const int BAR_HEIGHT_MIN = 5;


    //Constructor
    Visualizer::Visualizer(int bins, int width, int height, int sampleRate, int fftSize)
    : numBins(bins), screenWidth(width), screenHeight(height), sampleRate(sampleRate), fftSize(fftSize)
    {
        int BAR_HEIGHT_MAX = height-(0.1f*height); //Make it reach till the top 10%

        // Calculate bin width in Hz (useful for mapping frequencies)
        binWidth = static_cast<float>(sampleRate) / fftSize;

        //Creates Scene (simplifys work)
        if (!createScene()) {
            throw std::runtime_error("Failed to Create Scene (Missing or Invalid Data).");
        } 
    }


    /*
        Allocates proper size to bins and creates soundbars
        Returns: true if created, false otherwise
    */
    bool Visualizer::createScene()
    {   
        // Return error if necessary values are undefined or invalid
        if (sampleRate <= 0 || fftSize <= 0 || screenWidth <= 0 || screenHeight <= 0 || numBins <= 0)
            return false;
    
        generateLogBins();
        return true; // Success
    }

    void Visualizer::generateLogBins()
    {
        float fMin = 20.0f;  // Minimum frequency we care about
        float fMax = sampleRate / 2.0f;  // Nyquist limit (limit calcuable)
        
        //Clean start
        logBins.clear();
        soundBars.clear();
        float binPixels = (screenWidth / numBins);

        //I like a little bit of seperation
        float binSeparation = binPixels * 0.1f;  // 10% spacing
        binPixels *= 0.9f;  // Adjust bin width accordingly
    
        
        for (int i = 0; i < numBins; i++)
        {
            //Associated frequencies for each box
            float binFreq = fMin * pow(fMax / fMin, static_cast<float>(i) / (numBins - 1));
            binFreq = std::min(binFreq, sampleRate / 2.0f); //I think?
            logBins.push_back(binFreq);
    
            // Create the regions for each bar (evenly spaced)
            SDL_FRect bar = { i * (binPixels + binSeparation) + binSeparation / 2, 0, binPixels, BAR_HEIGHT_MIN };
            soundBars.push_back(bar);
        }


        
    }
    



    /*
        This uses the bin index to sum the amplitudes of the frequencies that belong to that bin.
        Args: Bin index we want to process.
        Returns: Amplitude of the bin.
    */
    float Visualizer::sumAmplitude(int binIndex) {
        // Error checking: Ensure binIndex is valid
        if (binIndex < 0 || binIndex >= numBins || freqData.empty()) {
            return 0.0f;  // Return zero if binIndex is out of range or FFT data is unavailable.
        }

        // Get the frequency range for this bin using logBins
        float freqStart = logBins[binIndex];
        float freqEnd = (binIndex < numBins - 1) ? logBins[binIndex + 1] : sampleRate / 2.0f;

        float amplitude = 0.0f;

        // Convert frequency range to FFT index range
        int startIdx = static_cast<int>(freqStart / binWidth);
        int endIdx = static_cast<int>(freqEnd / binWidth);

        // Ensure index bounds
        startIdx = std::max(0, std::min(startIdx, fftSize / 2 - 1));
        endIdx = std::max(0, std::min(endIdx, fftSize / 2 - 1));

        // Sum the magnitudes of FFT bins within the range
        for (int i = startIdx; i <= endIdx; i++) {
            amplitude += std::abs(freqData[i]); // Take magnitude from complex number
        }

        return amplitude;
    }


    /*
        Responsible for updating the visualizer. Pulls updated frequency from 'insertFreq()'
        Returns: 0 on success, else error 
    */
    int Visualizer::update(const std::vector<Complex>& input)
    {

        float maxAmplitude = 1.0f; // Prevent division by zero, can be dynamically updated
        std::vector<float> amplitudes(numBins, 0.0f); // Store computed amplitudes

        // First pass: Compute amplitudes and track max
        for (int i = 0; i < numBins; i++)
        {
            amplitudes[i] = sumAmplitude(i);
            if (amplitudes[i] > maxAmplitude)
            {
                maxAmplitude = amplitudes[i];
            }
        }

        // Second pass: Normalize and update bars
        for (int i = 0; i < numBins; i++)
        {
            float normalizedAmplitude = (maxAmplitude > 0) ? (amplitudes[i] / maxAmplitude) : 0.0f;
            soundBars[i].h = normalizedAmplitude * BAR_HEIGHT_MAX;
        }

        return 0;
    }

    //Render the updated graphics
    void Visualizer::render(SDL_Renderer* renderer)
    {
        // 🎨 Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
        SDL_RenderClear(renderer);

        // 🎶 Draw each sound bar
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green bars
        for (const auto& bar : soundBars) {
            SDL_RenderFillRect(renderer, &bar); // Draw bar
        }

        // 📤 Present the final frame
        SDL_RenderPresent(renderer);
    }

    //Destructor
    Visualizer::~Visualizer() {
        //doesnt do anything
    }
    

   

