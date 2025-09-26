#include <iostream>
#include <SDL3/SDL.h>
#include "glad/glad.h"


struct game {


}

struct platform {

    struct game* g;
}

struct object {


}


void log(char* s)       { std::cout << s << std::endl; }
void log(std::string s) { std::cout << s << std::endl; }

unsigned int compileShader(char** source, GLenum type) {

    unsigned int shaderLoc = glCreateShader(type);
    glShaderSource(shaderLoc, 1, source, NULL);
    glCompileShader(shaderLoc);

    int success; char infoLog[512];
    glGetShaderiv(shaderLoc, GL_COMPILE_STATUS, &success);
    if (!success) { 
        glGetShaderInfoLog(shaderLoc, 512, NULL, infolog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
}


// We could make this into SDL3's AppInit functions too but a proper main is better for understanding i think...


// Renderer
    // List of shaders & Last save date.

int main(int argc, char* argv[]) {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Turn on double buffering and request a 24-bit depth buffer
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow("SDL3 + OpenGL + GLAD Example", 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (SDL_GL_MakeCurrent(window, glContext) < 0) {
        std::cerr << "Failed to make OpenGL context current! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    
    std::cout << "OpenGL Version: " << GLVersion.major << "." << GLVersion.minor << std::endl;
    std::cout << "Vendor: " << (char*)glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << (char*)glGetString(GL_RENDERER) << std::endl;

    
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f); 

    SDL_GL_SetSwapInterval(1); // vsync on

    bool quit = false;
    SDL_Event event;

    while (!quit) {
        
        // ------ INPUT ---------//
        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_EVENT_QUIT)                                     { quit = true; }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) { quit = true; }

            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                SDL_GetWindowSize(window, &width, &height);
                glViewport(0, 0, width, height);
            }
        }

        // Stat each shader and check if stat value is new.
            // if so recompile the shader.
            // this will also explain the chaos of ue5's shader recompile.

        // -------- LOGIC ----------- //
        // nothing so far.. we are poor on logic.



        // --------------- RENDER --------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



        // swap active render buffr.
        SDL_GL_SwapWindow(window);
    }

    // pointless cleanup... but avoids valgrind errors.
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}