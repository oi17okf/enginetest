#include <iostream>
#include <filesystem>
#include <map>

#define MATH_3D_IMPLEMENTATION
#include "math_3d.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_image.h>
#include "glad/glad.h"


const std::string textureDir = "./textures/"
const std::string objectDir  = "./objects/"
const std::string shaderDir  = "./shaders/"
const std::string atlasLoc   = "./fontatlas.png"

const int   ascii_offset = -32; // Space has ascii index 32, altas start with Space.
const float uv_partition = 0.1f // Atlas is 10x10, thus each uv is a multiple of 0.1.

GLuint fontAtlasTextureID = 0;
GLuint textVAO, textVBO;

enum class GameMode { MENU, GAME };
GameMode gamemode = GameMode::MENU;

enum class MenuItem { PLAYLEVEL1, PLAYLEVEL2, PLAYLEVEL3, SPLITSCREEN, QUIT };
MenuItem active_menu_item = MenuItem::PLAYLEVEL1;

struct simple_mesh_data {

    std::string texture;
    std::string obj;

};

struct complex_mesh_data {

    std::string tex1;
    std::string tex2;
    std::string obj;
    std::string shader_vs;
    std::string shader_fs;

};


std::map<std::string name, simple_mesh_data> simple_mesh_map {

    { "cube",   { "cube_tex",  "cube.obj" }   },
    { "goomba", { "oomba_tex", "goomba.obj" } },
    { "roomba", { "oomba_tex", "roomba.obj" } },
    { "bench",  { "bench_tex", "bench.obj" }  },
    { "sword",  { "texture",   "sword.obj" }  },

};

//list of bound textures
//specific bound texture for text rendering

struct simple_object {

    // location
    // object id aka model to render
    // texture id
    // and implicit shader that will be used...

    struct* collision = nullptr;

}

struct textUI {

    std::string text;
    //vector2 pos
    int font_size;
    int color;
    //implicitly bound to specific textshader.

};

struct text3D {

    std::string text;
    //vector3 pos
    int font_size;
    int color;
    //implicitly bound to different shader than normal text.

};

struct level {

    int start_point;
    int goal;
    // static objects // here we can do static opengl arrays
    // moveable objects (they have a path they follow)
    // speed boosts
    // enemies1
    // enemies2 (or make enemies a map?)

}

enum class ShaderType { VS, FS };

struct shader {

    GLenum type; // GL_FRAGMENT_SHADER or GL_VERTEX_SHADER
    date       last_change;
    bool       dirty;
    std::string source;
    GLuint shader;

};

// Load all shaders of all types, and store them in a map of shaders with their name as key.
// A program is then defined as a combo of a vs shader and a fs shader.
// PROGRAMX = SHADER1.vs SHADER2.fs
// on runtime, we will know if we are missing a shader. if so abort. (can improve later)
// go through each program, link the needed shaders for them, and save the working programs.
// then the programs are then just stored with their name attached to it.
// for each frame we then do these steps
    // check shader sources for updated file, if so recompile it and mark shader as dirty.
    // go through each shaderprogram and see if they use a dirty shader, if so re-link
    // clear dirty flags in shader

// Then when rendering each program then has custom code related to it

// RenderAllUIText
    // setup correct shaderprogram, 
    // setup uniforms that changed since last frame, but is shared among all texts (not sure if any)
    // for each item
        // setup correct vertex data
        // update specific uniform (color?)

struct shader_program {

    GLuint ID = 0;
    struct shader* vs;
    struct shader* fs;
    std::map<std::string, GLuint> uniform_locations;
    //uniform locations are achieved through opengl reflection.
    
};

// Search through objects folder.
// go through each obj file there and parse them.
// each obj is then saved in struct below, where every found type is given it's own list with a name.

std::map<std::string, std::map<std::string, std::vector<std::float>>> objects;

objects["cube"].
struct obj_data {
    std::map<std::string, std::vector<std::string>> data;
}



// for level editor stuff, just implement raycast on collision box,
// then have a right-click context menu that allows one to edit entity settings.
// (location, rotation, scale, turn on/off collision, change mesh, delete)
// also have default actions like spawn object (selects based on entity type), save scene, scene settings (like lightning?)

//to make this work, have all types of objects have a pointer to 

struct special_ingame_object {


    struct object* location;
}

struct player {
    
    float x; // x,y,z should be turned to vec3
    float y;
    float has_dashed;
    float can_jump;

};

struct player_input {

    float move_x       = 0;
    float move_y       = 0;
    int   jump         = 0;
    int   floating     = 0;
    int   dash         = 0;
    int   punch        = 0;
    float rotate_cam_x = 0.0f;
    float rotate_cam_y = 0.0f;
};

struct menu_input {

    int move   = 0;
    int action = 0;
}





int active_players = 1;



std::vector<struct shader> shaders;


// implement Draw Call Reordering 

// draw instances

// do basic terrain w custom collision

void log(char* s)       { std::cout << s << std::endl; }
void log(std::string s) { std::cout << s << std::endl; }

void cleanUp() {
    // Fill in cleanup if needed later, most just for removing memory errors.
}


void initializeFontAtlas() {

    SDL_Surface* surface = IMG_Load(atlasLoc.c_str());

    glGenTextures(1, &fontAtlasTextureID);
    glBindTexture(GL_TEXTURE_2D, fontAtlasTextureID);
}

// Out game will max have 50 textures, so it makes sense to just load all of them at startup and leave them on the GPU
std::map<std::string, GLuint> loadAllTextures() {

    std::map<std::string, GLuint> loadedTextures;
    
    if (!std::filesystem::exists(textureDir) || !std::filesystem::is_directory(textureDir)) {
        log("texture directory not found");
        return loadedTextures;
    }

    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(textureDir)) {

        if (!entry.is_regular_file()) { continue; }

        std::string filePath = entry.path().string();
        std::string fileName = entry.path().stem().string();


        SDL_Surface* surface = IMG_Load(filePath.c_str());
        if (surface == nullptr) { log("error loading " + filePath); continue; }
        
        GLenum format;
        int numColors = surface->format->BytesPerPixel;

             if (numColors == 4) { format = surface->format->Rmask == 0x000000ff ? GL_RGBA : GL_BGRA; } 
        else if (numColors == 3) { format = surface->format->Rmask == 0x000000ff ? GL_RGB  : GL_BGR;  } 

        else { log("wrong color format in " + filepath); SDL_DestroySurface(surface); continue; }
        
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


        // This is where we upload data to the GPU, implicitly bound to the current textureID cuz opengl sucks.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0,                          
                     format, GL_UNSIGNED_BYTE, surface->pixels);           
        glGenerateMipmap(GL_TEXTURE_2D);
        
        loadedTextures[fileName] = textureID;
        SDL_DestroySurface(surface); //Clean up CPU data since it's no longer needed.    
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    return loadedTextures;
}

unsigned int compileShader(char** source, GLenum type) {

    unsigned int shaderLoc = glCreateShader(type);
    glShaderSource(shaderLoc, 1, source, NULL);
    glCompileShader(shaderLoc);

    int success; 
    char infoLog[512];
    glGetShaderiv(shaderLoc, GL_COMPILE_STATUS, &success);

    if (!success) { 

        glGetShaderInfoLog(shaderLoc, 512, NULL, infolog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
}


SDL_Gamepad* getGamepad() {

    SDL_Gamepad* gamepad = nullptr;

    SDL_Gamepad** gamepads = SDL_GetGamepads();
    if (gamepads != NULL && gamepads[0] != null) {
        SDL_GamepadID instance_id = SDL_GetGamepadInstanceID(gamepads[0]);
        gamepad = SDL_OpenGameController(instance_id);
    }

    SDL_free(gamepads);

    return gamepad;
}

void check_default_events(SDL_Event& event) {

    switch (event.type) {

        case SDL_EVENT_QUIT: { quit = true; break; }
        case SDL_EVENT_WINDOW_RESIZED: {

            SDL_SetWindowSize(window, &width, &height); 
            glViewport(0, 0, width, height);
            break;
        }

        case SDL_EVENT_GAMEPAD_ADDED: {

            if (activeGamepad == nullptr) {

                SDL_GamepadID instance_id = event.gdevice.instance_id;
                activeGamepad = SDL_OpenGameController(instance_id);
                log("Controller connected");
            }

            break;
        }

        case SDL_EVENT_GAMEPAD_REMOVED: {

            if (activeGamepad != nullptr && event.gdevice.instance_id == SDL_GetGamepadInstanceID(activeGamepad)) {

                SDL_CloseGamepad(activeGamepad);
                activeGamepad = nullptr;
                log("Controller disconnected");
            }

            break;
        }
    }
}

void setupPlayers(int count) {

    for(int i = 0; i < count; i++) {
        //new player
        //vector.push
    }

}

void loadLevel(int level) {

    // double check and clear active memory

    // struct map = maps[level];

    // for map.cubes
}

// Renderer
// List of shaders & Last save date.

int main(int argc, char* argv[]) {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
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
    std::cout << "Vendor: "         << (char*)glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: "       << (char*)glGetString(GL_RENDERER) << std::endl;

    
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f); 

    SDL_GL_SetSwapInterval(1); // vsync on


    SDL_Gamepad* activeGamepad = getGamepad();
    if (activeGamepad != nullptr) { log("Gamepad connected at startup - splitscreen available"); }

    bool quit = false;
    SDL_Event event;

    while (!quit) {

        struct menu_input m_input;
        struct p_input[active_players]; //depends on splitscreen
        
        // ------ INPUT ---------//
        while (SDL_PollEvent(&event)) {

            check_default_events(event);

            if (GameMode::MENU) {

                if (event.type == SDL_EVENT_KEY_DOWN) {

                    switch (event.key.key) {
                    case SDLK_ESCAPE: { quit = true; break; }
                    case SDLK_1: { //debug remove l8er
                        if (activeGamepad != nullptr) { active_players = active_players == 1 ? 2 : 1; }
                        break;
                    }
                    case SDLK_UP:    { m_input.move   =  1; break;  }
                    case SDLK_DOWN:  { m_input.move   = -1; break;  }
                    case SDLK_SPACE:
                    case SDLK_ENTER: { m_input.action =  1; break; }
                    default: { break; } }
                }

            } else if (GameMode::GAME) {

                

                if (event.type == SDL_EVENT_KEY_DOWN) {

                    switch (event.key.key) {
                    case SDLK_SPACE: { p_input[0].jump = 1; break; }
                    case SDLK_SHIFT: { p_input[0].dash = 1; break; }
                    default: { break; } }
                }

                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {

                    switch (event.mbutton.button) {
                    case SDL_BUTTON_LEFT:  { p_input[0].punch = 1; break; }
                    case SDL_BUTTON_RIGHT: { p_input[0].punch = 1; break; }
                    default: { break; } }
                }

                if (event.type == SDL_EVENT_MOUSE_MOTION) {
                    if (event.mmotion.xrel > 0.01f || event.mmotion.xrel < -0.01f) {
                        p_input[0].rotate_cam_x = event.mmotion.xrel;
                    }
                    if (event.mmotion.yrel > 0.01f || event.mmotion.yrel < -0.01f) {
                        p_input[0].rotate_cam_y = event.mmotion.yrel;
                    }
                }   

                if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && active_players == 2) {
                    switch (event.gbutton.button) {
                    case SDL_GAMEPAD_BUTTON_A:              { p_input[1].jump = 1;  break; }
                    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:  { p_input[1].dash = 1;  break; }
                    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: { p_input[1].punch = 1; break; }
                    default: { break; } }
                }                                    

            }
            
        }

        if (gamemode == GameMode::GAME) {

            const Uint8* keystate = SDL_GetKeyboardState(NULL);
            if (keystate[SDL_SCANCODE_SPACE]) { p_input[0].floating  = 1; }
            if (keystate[SDL_SCANCODE_W])     { p_input[0].move_y   += 1; }
            if (keystate[SDL_SCANCODE_S])     { p_input[0].move_y   -= 1; }
            if (keystate[SDL_SCANCODE_A])     { p_input[0].move_x   += 1; }
            if (keystate[SDL_SCANCODE_D])     { p_input[0].move_x   -= 1; }

            // if splitscreen & controller active
            if (activeGamepad != null && active_players == 2 &&) {
                if (SDL_GetGamepadButton(activeGamepad, SDL_GAMEPAD_BUTTON_A)) {
                    p_input[1].floating = 1;
                }
            } 
        }

        // --- INPUT END --- //

        // Stat each shader and check if stat value is new.
            // if so recompile the shader.
            // this will also explain the chaos of ue5's shader recompile.

        // -------- LOGIC ----------- //
        if (gamemode == GameMode::MENU) {

            active_menu_item += m_input.move;
            if (m_input.press) {

                switch (active_menu_item) {
                case MenuItem::PLAYLEVEL1:  { setupPlayers(active_players); loadLevel(1); gamemode = GameMode::GAME; break; }
                case MenuItem::PLAYLEVEL2:  { setupPlayers(active_players); loadLevel(2); gamemode = GameMode::GAME; break; }
                case MenuItem::PLAYLEVEL3:  { setupPlayers(active_players); loadLevel(3); gamemode = GameMode::GAME; break; }
                case MenuItem::SPLITSCREEN: { active_players = active_players == 1 ? 2 : 1; break; }
                case MenuItem::QUIT:        { quit = true; break; } }
            }
        
        } else if (gamemode == GameMode::GAME) {

        } 
        // nothing so far.. we are poor on logic.



        // --------------- RENDER --------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (gamemode == GameMode::MENU) {
            // render some type of background mby?

            // render text for all menu options
            // render selected text with different color

        } else if (gamemode == GameMode::GAME) {

            // check if splitscreen, if so do rendering twice w different viewports...
            for (int i = 0; i < active_players; i++) {

                int startx = i       / active_players * width;
                int starty = i       / active_players * height;
                int endx   = (i + 1) / active_players * width;
                int endy   = (i + 1) / active_players * height;
                glViewport(startx, starty, endx, endy);

                //render terrain / background / skybox
                //render static platform objects
                //render enemies
                //render player

                //render any ui stuff specific to a player
                //glDisable(GL_DEPTH_TEST);
                //glDisable(GL_CULL_FACE);
                //ui stuff
                glEnable(GL_DEPTH_TEST); 
                glEnable(GL_CULL_FACE);
            } 

            glViewport(0, 0, width, height);
            //render any ui stuff not specific to a player
            // renderAllTextUI()
            //glDisable(GL_DEPTH_TEST);
            //glDisable(GL_CULL_FACE);



            glEnable(GL_DEPTH_TEST); 
            glEnable(GL_CULL_FACE);   

        } 

        // swap active render buffr.
        SDL_GL_SwapWindow(window);
    }

    // pointless cleanup... but avoids valgrind errors.
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}



