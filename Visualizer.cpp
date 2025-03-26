#include "Visualizer.h"
#include <thread> //should do good

//For Debug (tis the name)
#ifdef DEBUG
    #define DEBUG_PRINT(msg) std::cout << "DEBUG: " << msg << std::endl;
#else
    #define DEBUG_PRINT(msg)
#endif

static SDL_Window *window = NULL; //Where visualizer resides
static SDL_Renderer *renderer = NULL; //This is for drawing
static SDL_AudioStream *stream = NULL; //This is not actually important, DAW plays audio
    //We listen to data

//Necessary for audio / setup
SDL_AudioSpec wavSpec; //Specs of your setup, sample rate and channgels I believe?
Uint8* wavBuffer;   //Buffer of EVERY sample, 0-255
Uint32 wavLength; //How long is the audio?

std::vector<float> audioSamples; //We convert Uint8 to float for FFT

//Standard SDL Window stuff
const int WINDOW_HEIGHT = 480;
const int WINDOW_LENGTH = 640;

float computeSize = 64;
bool rainbow = false;

int styleArr[4];

//Its a stylistic thing more than anything practical
const int BAR_HEIGHT_MIN = 5;


    //Constructor
    Visualizer::Visualizer(int bins, int width, int height, int sampleRate, int fftSize, int style)
    : numBins(bins), screenWidth(width), screenHeight(height), sampleRate(sampleRate), fftSize(fftSize), style(style)
    { 

        this->screenHeight = static_cast<float>(height);
        DEBUG_PRINT("height " << screenHeight);
        BAR_HEIGHT_MAX = screenHeight * 0.9f; //Make it reach till the top 10%
        DEBUG_PRINT("BAR_HEIGHT_MAX " << BAR_HEIGHT_MAX);

        // Calculate bin width in Hz (useful for mapping frequencies)
        binWidth = static_cast<float>(sampleRate) / fftSize;

        DEBUG_PRINT("Creating Scene");
        //Creates Scene (simplifys work)
        if (!createScene()) {
            throw std::runtime_error("Failed to Create Scene (Missing or Invalid Data).");
        } 


        /*
            Color Setup
        */
        switch (style) {
            case 1:
                // purple
                styleArr[0] = 155;
                styleArr[1] = 30;
                styleArr[2] = 200;
                styleArr[3] = 255; 
                break;
            case 2:
                // rainbow here
                styleArr[0] = 0;
                styleArr[1] = 0;
                styleArr[2] = 0;
                styleArr[3] = 0; 

                rainbow = true;
                break;
            default:
                // default to white
                styleArr[0] = 255;
                styleArr[1] = 255;
                styleArr[2] = 255;
                styleArr[3] = 255; 

                break;
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
    
        //This is my setup
        DEBUG_PRINT("Creating Log Bins / Soundbars");
        generateLogBins();

        DEBUG_PRINT("Beginning SDL Setup");
        createSDLComponents();

    
        return true; // Success
    }

    bool Visualizer::createSDLComponents() {
        /*
            Perform SDL Setup (RATHER than in main)        
        */
       DEBUG_PRINT("Initializing");
       //Initial Setup
       if (!SDL_Init(SDL_INIT_VIDEO)) { //Removed Audio since DAW is responsible for that
           SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
           return false;
       }
       DEBUG_PRINT("Creating Window");
       if (!SDL_CreateWindowAndRenderer("Visualizer Test", WINDOW_LENGTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
        //Passes references for window and renderer
           SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
           return false;
       }
       
       return true;

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
    
        DEBUG_PRINT("Entering Logbin Loop");
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


        DEBUG_PRINT("Successfully created sound bars and logbins");
        DEBUG_PRINT("Size of SoundBars: " << soundBars.size());
        DEBUG_PRINT("Size of logBins: " << logBins.size());
        
    }
    



    /*
        This uses the bin index to sum the amplitudes of the frequencies that belong to that bin.
        Args: Bin index we want to process.
        Returns: Amplitude of the bin.
    */
    float Visualizer::sumAmplitude(int binIndex) {
        DEBUG_PRINT("Performing Sum Amplitude");
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
        DEBUG_PRINT("Index Range " << startIdx << " " << endIdx );

        // Sum the magnitudes of FFT bins within the range
        for (int i = startIdx; i <= endIdx; i++) {
            DEBUG_PRINT("Real: " << freqData[i].real() << " Imaginary: " << freqData[i].imag());
            amplitude += std::abs(freqData[i]); // Take magnitude from complex number
        }

        DEBUG_PRINT("Amplitude: " << amplitude);
        return amplitude;
    }


    /*
        Responsible for updating the visualizer. Pulls updated frequency from 'insertFreq()'
        Returns: 0 on success, else error 
    */
    int Visualizer::update(const std::vector<Complex>& input)
    {
        

        DEBUG_PRINT("Updating Visualizer");
        float maxAmplitude = 1.0f; // Prevent division by zero, can be dynamically updated
        std::vector<float> amplitudes(numBins, 0.0f); // Store computed amplitudes

        DEBUG_PRINT("INPUT ------ Real: " << input[0].real() << " Imaginary: " << input[0].imag());
    
        freqData = input;

        // First pass: Compute amplitudes and track max
       for (int i = 0; i < numBins; i++)
        {
            amplitudes[i] = sumAmplitude(i);
            DEBUG_PRINT("Amplitude at " << i << " ---- " << amplitudes[i]);
            if (amplitudes[i] > maxAmplitude)
            {
                maxAmplitude = amplitudes[i];
            }
        }

        DEBUG_PRINT("BAR HEITGHT MAX: " << BAR_HEIGHT_MAX);
        
        // Second pass: Normalize and update bars
        for (int i = 0; i < numBins; i++)
        {
            float normalizedAmplitude = (amplitudes[i] / (maxAmplitude + 0.1f));  // Add small constant to prevent division by zero

            // You may want to remove the DEBUG_PRINT here for performance reasons in production
            DEBUG_PRINT("Current Amplitude: " << normalizedAmplitude)

            soundBars[i].h = BAR_HEIGHT_MIN + (normalizedAmplitude) * BAR_HEIGHT_MAX;
            DEBUG_PRINT("HEIGHT FOR " << i << " = " << soundBars[i].h);
        }

        


        return 0;
    }

    //Render the updated graphics
    void Visualizer::render(SDL_Renderer* renderer)
    {

        // Refreshes scene
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
        SDL_RenderClear(renderer);

        // Draw the soundbars
        SDL_SetRenderDrawColor(renderer, styleArr[0],  styleArr[1],  styleArr[2],  styleArr[3]); // now custom and cool
        for (const auto& bar : soundBars) {
            SDL_RenderFillRect(renderer, &bar);
        }

        SDL_RenderPresent(renderer);
    }

    //Destructor
    Visualizer::~Visualizer() {
        free(wavBuffer);

        if (window) SDL_DestroyWindow(window);
        if (renderer) SDL_DestroyRenderer(renderer);

        SDL_Delay(10000);
        SDL_Quit();

    }

    //These directly convert which is nice, but we have to do this anyways.
    void convertUint8ToFloat(Uint8* buf){
        for(size_t i = 0; i < wavLength; i++)
        {
            audioSamples.push_back(float(wavBuffer[i]));
        }

        free(buf);
    }

    /*
        This will run the specified visualizer, so if you create a visualizer and want to demonstrate it, this is how you can do it.
    */
    bool runVisualizer(Visualizer& visualizer){

        DEBUG_PRINT("Entering Main Runner");
        using Complex = std::complex<float>;
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        
        //App control flow
        bool exit = false;
        SDL_Event event;
        
        bool isInitialized = false; 
        int current;  
        int increment = computeSize; // To ttest, will change in future!
        
            
        while (!exit) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    exit = true; // Exit on window close
                }
            
                // Initialize the variable the first time
                if (!isInitialized) {
                    
                    current = 0;  
                    FFT::setup(audioSamples);
                    DEBUG_PRINT("Finihed Setup");
                    isInitialized = true;  
                } else {
                    current += increment/2;
                }

                std::vector<Complex> fftData = FFT::compute(current, increment);
                visualizer.update(fftData);
                visualizer.render(renderer); 
            }

            /*
                REMOVED FEATURES:
                -Timer >> Since we're relying on updates from DAW, we don't need a timer for consistency
            */
        }   

        return true;
    }
    
    // We need an easier interface to actually update the audio
    void populateBuffer(){
        //First we simply insert raw audio 
        //then we can convert to the usable type. Need to verify how the actual stuff comes out.
        
    }

    //Testing
    int main() {
        /*
            Tips for using this: 
            Visualizer uses visualzier(16,WL,WH,samplerate,FFT)
            For FFT number, make sure it is atleast 4 time the number of bins
            for an accurate representation

            Style is a number from 0-2, 0 being white, 1 being purple, and 2 being rainbow. 
            subject to change
        */

        
        Visualizer visualizer(16, WINDOW_LENGTH, WINDOW_HEIGHT, computeSize, 64, 1);
        
        //Random buffer, nonsense
        audioSamples = {132, 146, 236, 241, 112, 210, 133, 7, 239, 253, 147, 90, 220, 247, 8, 18, 31, 32, 182, 249, 140, 75, 216, 171, 145, 61, 242, 56, 233, 147, 89, 228, 69, 119, 202, 186, 186, 208, 81, 98, 166, 138, 128, 43, 90, 96, 171, 80, 142, 91, 130, 187, 104, 177, 23, 100, 5, 192, 66, 106, 128, 9, 128, 250, 136, 137, 158, 94, 241, 224, 187, 164, 172, 25, 46, 116, 179, 68, 152, 29, 193, 215, 98, 241, 122, 229, 117, 162, 77, 229, 255, 234, 62, 113, 207, 204, 155, 233, 241, 14, 241, 159, 230, 190, 6, 254, 187, 87, 21, 134, 218, 157, 213, 100, 42, 167, 99, 226, 55, 176, 14, 131, 158, 44, 155, 204, 21, 5, 108, 161, 26, 176, 78, 233, 4, 202, 6, 52, 128, 25, 133, 135, 249, 211, 226, 27, 195, 77, 
            137, 170, 22, 14, 138, 120, 14, 117, 235, 103, 235, 102, 195, 167, 65, 19, 36, 69, 109, 207, 11, 40, 17, 138, 142, 232, 236, 102, 139, 213, 203, 115, 62, 85, 92, 162, 94, 
            239, 190, 27, 124, 227, 2, 241, 151, 252, 116, 180, 240, 180, 52, 190, 101, 168, 232, 105, 40, 35, 212, 207, 114, 125, 99, 255, 160, 228, 60, 189, 232, 177, 62, 31, 3, 131, 44, 8, 104, 126, 211, 55, 47, 206, 80, 3, 201, 45, 128, 25, 198, 23, 53, 190, 90, 85, 176, 227, 99, 80, 166, 71, 129, 176, 87, 71, 84, 140, 67, 153, 95, 176, 203, 30, 71, 0, 221, 73, 25, 162, 63, 232, 182, 98, 96, 236, 164, 37, 8, 238, 80, 129, 243, 161, 95, 93, 209, 245, 153, 14, 217, 220, 131, 249, 175, 4, 15, 45, 240, 62, 116, 58, 34, 
            187, 82, 181, 47, 203, 202, 241, 26, 76, 197, 151, 54, 194, 254, 239, 147, 13, 119, 70, 243, 59, 155, 3, 243, 239, 29, 191, 75, 137, 10, 196, 192, 246, 52, 49, 175, 3, 44, 55, 169, 183, 245, 56, 9, 227, 207, 139, 76, 179, 191, 91, 134, 85, 186, 166, 114, 150, 10, 77, 149, 119, 122, 194, 78, 65, 100, 90, 211, 0, 180, 10, 232, 184, 226, 41, 123, 181, 66, 74, 32, 62, 253, 70, 63, 204, 225, 66, 57, 189, 155, 27, 166, 117, 98, 133, 86, 226, 188, 198, 64, 134, 211, 48, 191, 23, 137, 0, 227, 235, 70, 69, 160, 59, 144, 117, 88, 224, 22, 194, 53, 22, 215, 20, 181, 89, 28, 203, 222, 224, 6, 3, 253, 104, 244, 171, 117, 252, 45, 100, 34, 53, 180, 68, 203, 249, 27, 20, 34, 230, 36, 167, 74, 246, 41, 139, 59, 162, 181, 134, 219, 62, 117, 169, 60, 227, 5, 8, 5, 234, 203, 184, 189, 8, 255, 220, 19, 56, 35, 60, 131, 141, 75, 4, 32, 55, 145, 109, 146, 154, 3, 148, 45, 151, 11, 15, 14, 190, 184, 88, 172, 125, 213, 0, 3, 119, 154, 243, 140, 255, 139, 139, 251, 46, 127, 56, 195, 79, 204, 45, 125, 161, 39, 163, 4, 230, 66, 117, 50, 
            76, 178, 74, 127, 142, 142, 208, 210, 147, 158, 183, 27, 151, 187, 109, 211, 199, 184, 35, 28, 105, 112, 100, 175, 48, 176, 158, 27, 72, 18, 143, 244, 91, 147, 78, 253, 0, 7, 56, 153, 251, 67, 239, 212, 221, 206, 52, 192, 230, 242, 12, 235, 144, 251, 113, 200, 232, 164, 233, 181, 247, 246, 16, 46, 201, 20, 4, 78, 123, 27, 234, 227, 121, 199, 146, 28, 94, 117, 4, 248, 22, 232, 70, 126, 57, 31, 128, 55, 31, 108, 32, 100, 166, 165, 237, 121, 206, 90, 123, 220, 236, 175, 7, 60, 61, 129, 246, 21, 106, 22, 179, 238, 125, 255, 17, 172, 224, 216, 95, 30, 59, 18, 219, 152, 55, 213, 153, 204, 240, 23, 140, 52, 251, 205, 86, 150, 116, 244, 108, 243, 63, 245, 218, 71, 102, 74, 225, 145, 
            92, 123, 28, 159, 107, 93, 11, 239, 204, 68, 11, 59, 66, 93, 106, 16, 90, 122, 164, 216, 212, 109, 27, 47, 167, 137, 234, 3, 97, 237, 4, 155, 41, 146, 34, 59, 17, 248, 162, 213, 73, 14, 181, 207, 84, 48, 210, 225, 164, 89, 121, 157, 47, 128, 15, 254, 240, 227, 62, 131, 176, 193, 219, 224, 79, 126, 10, 185, 221, 212, 163, 159, 244, 198, 210, 40, 171, 191, 23, 77, 228, 226, 205, 83, 142, 28, 232, 87, 176, 20, 98, 244, 244, 142, 244, 235, 98, 191, 29, 45, 62, 39, 100, 205, 81, 251, 241, 151, 223, 220, 136, 135, 123, 229, 172, 73, 63, 229, 176, 247, 253, 218, 244, 129, 205, 6, 239, 126, 114, 202, 109, 195, 121, 212, 36, 190, 37, 90, 245, 250, 194, 196, 208, 78, 143, 91, 13, 160, 
            2, 7, 112, 56, 71, 54, 183, 211, 129, 144, 8, 213, 0, 142, 162, 46, 45, 181, 209, 22, 185, 186, 162, 178, 200, 151, 35, 202, 163, 90, 251, 152, 57, 189, 172, 160, 127, 5, 
            120, 139, 239, 112, 164, 87, 80, 229, 191, 189, 46, 74, 154, 83, 183, 135, 188, 55, 222, 10, 64, 183, 112, 254, 191, 221, 104, 91, 23, 181, 65, 254, 120, 144, 88, 87, 162, 4, 98, 48, 170, 16, 40, 244, 49, 84, 230, 64, 36, 25, 200, 91, 119, 225, 253, 201, 231, 119, 20, 71, 79, 250, 166, 78, 48, 62, 160, 63, 110, 83, 154, 43, 233, 172, 237, 45, 176, 241, 49, 156, 226, 96, 101, 250, 69, 217, 95, 108, 131, 119, 74, 4, 73, 195, 12, 170, 183, 209, 11, 225, 47, 59, 110, 70, 156, 79, 201, 65, 62, 27, 110, 107, 142, 
            20, 211, 231, 237, 54, 192, 124, 42, 213, 92, 32, 93, 98, 252, 60, 86, 229, 178, 44, 222, 82, 141, 208, 30, 11, 188, 28, 37, 194, 158, 252, 232, 20, 178, 112, 81, 235, 140, 104, 180, 78, 41, 249, 219, 171, 227, 13, 226, 186, 196};
            
        runVisualizer(visualizer);

        SDL_Delay(10000);
    }
   
