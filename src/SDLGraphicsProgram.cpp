#include "SDLGraphicsProgram.hpp"
#include "Camera.hpp"
#include "Sphere.hpp"
#include "Terrain.hpp"

#include <dr_wav.h>
#include <miniaudio.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// Minaudio and dr_wav tools
ma_engine engine;
drwav wav;

// Scene objects and nodes
Terrain* gTerrain;
SceneNode* gTerrainNode;
Object* gObj;
SceneNode* gNode;

// Used to generate outer sphere
float maxAmplitude = 0.0f;

// Initialization function
// Returns a true or false value based on successful completion of setup.
// Takes in dimensions of window.
SDLGraphicsProgram::SDLGraphicsProgram(int w, int h, const char* filepath) {
	// Initialization flag
	bool success = true;
	// String to hold any errors that occur.
	std::stringstream errorStream;
	// The window we'll be rendering to
	m_window = NULL;

    // Store path to audio file
    m_filepath = filepath;

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		errorStream << "SDL could not initialize! SDL Error: " << SDL_GetError() << "\n";
		success = false;
	} else {
		// Use OpenGL 3.3 core
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		// We want to request a double buffer for smooth updating.
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

		// Create window
		m_window = SDL_CreateWindow("Project",
                                SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED,
                                w,
                                h,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

		// Check if Window did not create.
		if (m_window == NULL){
			errorStream << "Window could not be created! SDL Error: " << SDL_GetError() << "\n";
			success = false;
		}

		// Create an OpenGL Graphics Context
		m_openGLContext = SDL_GL_CreateContext(m_window);
		if  (m_openGLContext == NULL){
			errorStream << "OpenGL context could not be created! SDL Error: " << SDL_GetError() << "\n";
			success = false;
		}

		// Initialize GLAD Library
		if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
			errorStream << "Failed to iniitalize GLAD\n";
			success = false;
		}

		// Initialize OpenGL
		if (!InitGL()) {
			errorStream << "Unable to initialize OpenGL!\n";
			success = false;
		}
  	}

    // If initialization did not work, then print out a list of errors in the constructor.
    if (!success) {
        errorStream << "SDLGraphicsProgram::SDLGraphicsProgram - Failed to initialize!\n";
        std::string errors=errorStream.str();
        SDL_Log("%s\n", errors.c_str());
    } else {
        SDL_Log("SDLGraphicsProgram::SDLGraphicsProgram - No SDL, GLAD, or OpenGL, errors detected during initialization\n\n");
    }

    // Optional: extra debug support
	// SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);

	GetOpenGLVersionInfo();

    // Setup our Renderer
    m_renderer = new Renderer(w, h);

    // Initialize miniaudio engine
    ma_result result;
    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        std::cout << "Could not initialize miniaudio engine" << std::endl;
        ma_engine_uninit(&engine);
        return;
    }

    // Open the WAV file for reading
    if (!drwav_init_file(&wav, m_filepath, NULL)) {
        std::cout << "Could not open WAV file" << std::endl;
        // Handle error
        return;
    }
}


// Proper shutdown of SDL and destroy initialized objects
SDLGraphicsProgram::~SDLGraphicsProgram() {
    if (m_renderer != nullptr){
        delete m_renderer;
    }

    if (gTerrain != nullptr){
        delete gTerrain;
    }

    if (gTerrainNode != nullptr){
        delete gTerrainNode;
    }

    if (gObj != nullptr){
        delete gObj;
    }

    if (gNode != nullptr){
        delete gNode;
    }

    // Destroy window
	SDL_DestroyWindow(m_window);
	// Point m_window to NULL to ensure it points to nothing.
	m_window = nullptr;
	// Quit SDL subsystems
	SDL_Quit();
}


// Initialize OpenGL
// Setup any of our shaders here.
bool SDLGraphicsProgram::InitGL() {
	// Success flag
	return true;
}

void SDLGraphicsProgram::Loop() {
    // Initialize scene
    // Terrain
    float terrainSize = 256;
    gTerrain = new Terrain((int) terrainSize, (int) terrainSize, "./textures/terrain.ppm");
    gTerrain->LoadTexture("./textures/colormap.ppm");
    gTerrainNode = new SceneNode(gTerrain);
    m_renderer->setRoot(gTerrainNode);
    // Sphere
    gObj = new Sphere();
    gObj->LoadTexture("./textures/sun.ppm");
    gNode = new SceneNode(gObj);
    gTerrainNode->AddChild(gNode);

    // Set a default position for our camera
    m_renderer->GetCamera(0)->SetCameraEyePosition(terrainSize / 2.0f, terrainSize / 4.0f, terrainSize * 1.25f);

    // Main loop flag
    // If this is quit = 'true' then the program terminates.
    bool quit = false;
    // Event handler that handles various events in SDL
    // that are related to input and output
    SDL_Event e;
    // Enable text input
    SDL_StartTextInput();

    // Set the camera speed for how fast we move.
    float cameraSpeed = 5.0f;

    // Retrieve data for first point in audio file
    int startTick = SDL_GetTicks();
    int prevTick = startTick;
    float amplitude = 0;
    float progress = 0.0f;
    char baseColor = 'G';

    // Play audio file
    ma_engine_play_sound(&engine, m_filepath, NULL);

    // While application is running
    while (!quit) {

        // Retrieve data for current point in audio file
        int curTick = SDL_GetTicks();
        int timeElapsed = curTick - startTick;
        int duration = curTick - prevTick;
        prevTick = curTick;

        // Retrieve the sample rate and total sample count
        uint64_t sampleRate = wav.sampleRate;
        uint64_t totalSamples = wav.totalPCMFrameCount;

        // Compute the sample index corresponding to the current point in time
        drwav_uint64 sampleIndex = (drwav_uint64) ((float) timeElapsed / 1000.0f * sampleRate);

        // Calculate the elapsed percentage
        progress = (float) sampleIndex / totalSamples;

        // Seek to the corresponding sample index
        drwav_seek_to_pcm_frame(&wav, sampleIndex);

        // Read the sample value at the current sample index
        drwav_int32 sample;
        drwav_read_pcm_frames_s32(&wav, 1, &sample);

        // Normalize the sample value to the range [-1.0, 1.0]
        amplitude = (float) sample / (float) INT32_MAX;
        
        // Initialize transform for nodes in scene
        gTerrainNode->GetLocalTransform().LoadIdentity();
        gNode->GetLocalTransform().LoadIdentity();

        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // User posts an event to quit
            // An example is hitting the "x" in the corner of the window.
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            // Handle keyboard input for the camera class
            if (e.type == SDL_MOUSEMOTION) {
                // Handle mouse movements
                int mouseX = e.motion.x;
                int mouseY = e.motion.y;
                m_renderer->GetCamera(0)->MouseLook(mouseX, mouseY);
            }
            switch(e.type) {
                // Handle keyboard presses
                case SDL_KEYDOWN:
                    switch(e.key.keysym.sym){
                        case SDLK_LEFT:
                            m_renderer->GetCamera(0)->MoveLeft(cameraSpeed);
                            break;
                        case SDLK_RIGHT:
                            m_renderer->GetCamera(0)->MoveRight(cameraSpeed);
                            break;
                        case SDLK_UP:
                            m_renderer->GetCamera(0)->MoveForward(cameraSpeed);
                            break;
                        case SDLK_DOWN:
                            m_renderer->GetCamera(0)->MoveBackward(cameraSpeed);
                            break;
                        case SDLK_PAGEUP:
                            m_renderer->GetCamera(0)->MoveUp(cameraSpeed);
                            break;
                        case SDLK_PAGEDOWN:
                            m_renderer->GetCamera(0)->MoveDown(cameraSpeed);
                            break;
                        case SDLK_r:
                            baseColor = 'R';
                            break;
                        case SDLK_g:
                            baseColor = 'G';
                            break;
                        case SDLK_b:
                            baseColor = 'B';
                            break;
                    }
                break;
            }
        } // End SDL_PollEvent loop.

        float terrainCenter = terrainSize / 2.0f;

        gTerrainNode->GetLocalTransform().LoadIdentity();
        gTerrainNode->GetLocalTransform().Scale(1.0f, 1.0f, 1.0f);
        gTerrainNode->GetLocalTransform().Translate(terrainCenter, 0.0f, terrainCenter);
        gTerrainNode->GetLocalTransform().Rotate(progress * 2 * M_PI, 0.0f, 0.1f, 0.0f);
        gTerrainNode->GetLocalTransform().Translate(-terrainCenter, 0.0f, -terrainCenter);

        // Prevent sphere from scaling down to too small a size
        float size = std::min(20.0f, std::max(1.25f, amplitude * terrainSize / 4.0f));

        gNode->GetLocalTransform().LoadIdentity();
        gNode->GetLocalTransform().Translate(terrainCenter, terrainSize / 2.5f, terrainCenter);
        gNode->GetLocalTransform().Scale(size, size, size);
        gNode->GetLocalTransform().Rotate(progress * 2 * M_PI, 0.0f, 0.1f, 0.0f);

        // Update our scene through our renderer
        m_renderer->Update(amplitude, progress, baseColor);
        // Render our scene using our selected renderer
        m_renderer->Render();
        // Delay to slow things down just a bit!
        SDL_Delay(1);
      	// Update screen of our specified window
      	SDL_GL_SwapWindow(GetSDLWindow());

        if (progress >= 1.0f) {
            quit = true;
        }
	}

    drwav_uninit(&wav);
    ma_engine_uninit(&engine);

    // Disable text input
    SDL_StopTextInput();
}


// Get Pointer to Window
SDL_Window* SDLGraphicsProgram::GetSDLWindow(){
  return m_window;
}

// Helper Function to get OpenGL Version Information
void SDLGraphicsProgram::GetOpenGLVersionInfo(){
	SDL_Log("(Note: If you have two GPU's, make sure the correct one is selected)");
	SDL_Log("Vendor: %s", (const char*)glGetString(GL_VENDOR));
	SDL_Log("Renderer: %s", (const char*)glGetString(GL_RENDERER));
	SDL_Log("Version: %s", (const char*)glGetString(GL_VERSION));
	SDL_Log("Shading language: %s", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
}