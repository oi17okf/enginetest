#include <iostream>
#include <filesystem>
#include <map>

#define MATH_3D_IMPLEMENTATION
#include "math_3d.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_image.h>
#include "glad/glad.h"

static inline radians(float deg) { return deg * M_PI / 180.0f; }


// integrate math_3d.h
// get the menu system up and running so that we can select a level.
// make it so a player (sphere) can walk across terrain on a level w a camera that can be moved by the mouse.
// integrate start & goal along w a timer / highscore. (if difficult, ignore visualizing timer)
// add in static objects w collisions
// integrate splitscreen
// make player controls a bit fun along w adding legs / simple animations
// add in moving platforms, enemies, bouncers along w something interactable
// update plan after this

// ONS showcase

// ACTION SYSTEM ACTIONS - Z or X is used in conjunction with mouse.
// TAB or SHIFT_TAB is used to shift between modes.
// Objects pos, scale, rot is changed by modifying text file for simplicity sake
/*

Clear Selection - No input
Select Terrain  - No input, only selected Z to inc, X to decr (based on where mouse is)
Create Object   - No input
Select Object   - Int input


Main editor loop
switch on mouse ray hit
    terrain - set terrain selected. if Tab swap mode. if mouse pressed modify terrain. in scroll modify size.
    object - set specic object selected. if tab swap model of object.
    nothing

*/



// X. validate loadAllObjs file
// Z. manually create a level by writing in text file.
// Y. split into proper functions/files


const std::string textureDir = "./textures/";
const std::string objectDir  = "./objects/";
const std::string terrainDir = "./saves/terrains/";
const std::string shaderDir  = "./shaders/";
const std::string savesDir   = "./saves/";
const std::string dataDir    = "./data/";
const std::string atlasLoc   = "./fontatlas.png";


int screen_width, screen_height;

float deltaTime = 0.0f;

GLuint fontAtlasTextureID = 0;
GLuint textVAO, textVBO, textEBO;

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

};

std::map<std::string, struct text_info> text_infos; 

struct text_info {

    int pos;
    int len;
};

void initializeTexts() {

    const int   ascii_offset = -32; // Space has ascii index 32, altas start with Space.
    const int   atlas_size   = 10;
    const float uv_partition = 1.0f / (float)atlas_size;


    glGenVertexArrays(1, &textVAO);
    glGenBuffers     (1, &textVBO);
    glGenBuffers     (1, &textEBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBindBuffer(GL_ARRAY_BUFFER, textEBO);


    std::vector<float> vertex;
    std::vector<int>   index;
    int pos = 0; 

    for (int i = 0; i < game_strings.length(); i++) {

        string s = game_strings[i];

        for (int j = 0; j < s.length(); j++) {

            char c = s[j];
            int uv_y = (c + ascii_offset) / atlas_size;
            int uv_x = (c + ascii_offset) % atlas_size; 

            //bl, br, tr, tl
            float uv_x_1 = uv_x * uv_partition;
            float uv_x_2 = uv_x * uv_partition + uv_partition;
            float uv_y_1 = uv_y * uv_partition;
            float uv_y_2 = uv_y * uv_partition + uv_partition;

            float pos_x_1 = j;
            float pos_x_2 = j + 1;
            float pos_y_1 = 0;
            float pos_y_2 = 1;

            std::vector<float> new_vertexes = { pos_x_1, pos_y_2, uv_x_1, uv_y_2,  // bl
                                                pos_x_2, pos_y_2, uv_x_2, uv_y_2,  // br
                                                pos_x_2, pos_y_1, uv_x_2, uv_y_1,  // tr
                                                pos_x_1, pos_y_1, uv_x_1, uv_y_1 }; // tl

            vertex.insert(new_vertexes.begin(), new_vertexes.end());

            std::vector<int> new_indices = { 0, 1, 2, 0, 2, 3 };
            index.insert(new_indices.begin(), new_indices.end());

        }

        struct text_info info;
        info.pos = pos;
        info.len = s.length;

        pos += s.length;

        text_infos[s] = info;
    }






    //do some binding here

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (2 * sizeof(float)));


    glBindVertexArray(0);


}

std::vector<std::string> game_strings {

    "PLAY" ,
    "SPLITSCREEN",
    "QUIT",
    "START",
    "GOAL",
    "ENEMY",
    "LEVEL1",
    "LEVEL2",
    "LEVEL3"
};


//here we assume we are already in proper text/2d mode/correct shader.
void draw_2d_text(std::string text, int x, int y, int size, color c) {

    struct text_info t;
    try {
        t = text_infos.get(text);
    } catch (const std::out_of_range& e) {
        log("Could not find text " + text + " when drawing 2d - abort");
        SDL_Quit();
    }

    // try / get when retriving uniform locs
    glUniform3fv(colorLoc, 1, color);
    glUniform1i(sizeLoc, 1, size);
    glUniform2fv(x,y);

    // dont think we have to deal w texture 
    // since it should be set up correctly at startup?

    glBindVertexArray(textVAO);
    glDrawElements(GL_TRIANGLES, t.len, GL_UNSIGNED_INT, t.pos);


}

struct 3d_text {

    std::string text;
    color c;
    vec3_t pos;
    vec3_t rot;
    vec3_t scale;
};

void draw_3d_text(std::string s, int x, int y, int size, color c) {

    struct text_info t;
    try {
        t = text_infos.get(text);
    } catch (const std::out_of_range& e) {
        log("Could not find text " + text + " when drawing 2d - abort");
        SDL_Quit();
    }

    // try / get when retriving uniform locs
    glUniform3fv(colorLoc, 1, color);
    glUniform1i(sizeLoc, 1, size);
    glUniform2fv(x,y);

    // dont think we have to deal w texture 
    // since it should be set up correctly at startup?

    glBindVertexArray(textVAO);
    glDrawElements(GL_TRIANGLES, t.len, GL_UNSIGNED_INT, t.pos);


}

// these texts are potentially written to be entities that own them.
// could in theory use the text field to represent state/emotion
std::vector<struct 3d_text> 3d_texts = {
    { "goomba", BLUE, pos, rot, scale },
    { "boomba", BLUE, pos, rot, scale },
}

// RenderAllUIText
    // setup correct shaderprogram, 
    // setup uniforms that changed since last frame, but is shared among all texts (not sure if any)
    // for each item
        // setup correct vertex data
        // update specific uniform (color?)

struct level active_level;

struct level {

    vec3_t start_point;
    vec3_t goal;

    // static objects // here we can do static opengl arrays
    // moveable objects (they have a path they follow)
    // speed boosts
    // enemies1
    // enemies2 (or make enemies a map?)
    // terrain


    int level_info_ref;

};

enum class ShaderType { VS, FS };

struct shader {

    GLenum type; // GL_FRAGMENT_SHADER or GL_VERTEX_SHADER
    std::time_t       last_change;
    bool       dirty;
    std::string path;
    std::string name;
    std::string source;
    GLuint pos;

};

//double check these later.
std::map<std::string name, std::tuple<std::string, std::string>> shaderPrograms = {
    { "default", std::make_tuple("default_shader.vs",  "default_shader.fs")  },
    { "text2d",  std::make_tuple("shaders/phong.vert", "shaders/phong.frag") },
    { "text3d",  std::make_tuple("shaders/color.vert", "shaders/color.frag") }
};

std::map<std::string, struct shader> shaders;
std::map<std::string, struct shader_program> programs;



// Then when rendering each program then has custom code related to it



struct shader_program {

    GLuint pos = 0;
    std::string vs;
    std::string fs;
    std::map<std::string, GLuint> uniform_locations;  //uniform locations are achieved through opengl reflection.
    
};


struct renderable {

    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
    int size;
};

struct AABB {

    int min;
    int max;
    int center;
};

// we could calc color from height...
struct terrain {

    SDL_Surface* height;
    SDL_Surface* color;

    int dirty = 0; //could be used as index, if we only change 1 height vert at a time.
    std::string name;

    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
}

struct terrain active_terrain;


// https://www.learnopengles.com/android-lesson-eight-an-introduction-to-index-buffer-objects-ibos/
void loadTerrain(std::string file_name) {

    active_terrain.name   = file_name;
    active_terrain.height = IMG_Load(file_name + "_height.png");
    active_terrain.color  = IMG_Load(file_name + "_color.png");

    //assert that height & color are the same.
    int width  = active_terrain.height->w;
    int height = active_terrain.height->h;
    int bpp    = active_terrain.height->format->BytesPerPixel;
    int pitch  = active_terrain.height->pitch;

    std::vector<float> vertex;
    std::vector<int>   index;

    // While a visual map looks to represent a "tile",
    // it insteads represent the top left vertex of that tile.

    const float grid_size = 1.0f;

    for (int i = 0; i < height; i++) {

        for (int j = 0; j < width; j++) {

            //look at types here. Not sure if we index properly, given that we get uint8 to float.
            float height = active_terrain.height[(i * width + j) * bpp];
            // we should also normalize height somewhat?

            // Just input data for VBO here.
            vertex.push_back(j *  grid_size); // X
            vertex.push_back(i * -grid_size); // Y, grow down.
            vertex.push_back(height);

            float r = active_terrain.color[(i * width + j) * bpp];
            float g = active_terrain.color[(i * width + j) * bpp + 1];
            float b = active_terrain.color[(i * width + j) * bpp + 2];

            vertex.push_back(r); 
            vertex.push_back(g);
            vertex.push_back(b);

            // EBO
            if (i != 0) {

                int p1 = j + (i - 1) * width;
                int p2 = j + i       * width;

                // Extra index at start of row for degen tri.
                if (i > 1 && j == 0) {
                    index.push_back(p1);
                }

                index.push_back(p1);
                index.push_back(p2);

                if (i > 0 && i < height -1 && j == width - 1) {
                    index.push_back(p2);
                }

            }

        }

    }

    glGenVertexArrays(1, &terrain.VAO);
    glGenBuffers     (1, &terrain.VBO);
    glGenBuffers     (1, &terrain.EBO);

    glBindVertexArray(terrain.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, terrain.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, terrain.EBO);

    glBufferData(GL_ARRAY_BUFFER,         sizeof(vertices), vertices, GL_STATIC_DRAW); // Change when we make terrain editable
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices),  indices,  GL_STATIC_DRAW); // indices

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (3 * sizeof(float)));

    glBindVertexArray(0);


}

void cleanTerrain() {



}


void saveTerrain() {

    IMG_SavePNG(surface, filepath);
}

// here we mix save data and static data, mby not the best...
struct level_info {

    std::string visual_name;
    std::string file_name;
    float best_time;
    float gold_time;
    float silver_time
    float bronze_time;

};

std::vector<struct level_info> level_infos;


void loadLevelInfos() {

    level_infos.clear();

    std::ifstream medal_times_file(medal_times_path);
    std::string line;
    int line_count = 0;
    while (std::getline(medal_times_file, line)) {

        std::stringstream ss(line);
        struct level_info l;
        l.best_time = -1.0f;

        if (!(ss >> l.visual_name >> l.file_name >> l.gold >> l.silver >> l.bronze)) { 
            log("error loading medal times on line: " + std::to_string(line_count));
            line_count++;
            continue;
        } 

        std::string saved_time_file_name = PATH + l.file_name;
        if (fs::exists(saved_time_file_name)) {

            std::string saved_line;
            std::getline(saved_time_file_name, saved_line);
            std::stringstream ss1(saved_line);
            ss1 >> l.best_time; 

        } else {

            std::ofstream outFile(filename, std::ios::trunc);
            outFile << value;
        }


        line_count++;

    }

}

// here we could add some sanity checks.
void saveLevelTime(int i, float time) {

    level_infos[i].best_time = time;
    std::string file_name = SAVEFILEPATH + level_infos[i].file_name;

    std::ofstream outFile(filename, std::ios::trunc);
    outFile << value;

}

void loadLevelTime(std::string) {


}






// for level editor stuff, just implement raycast on collision box,
// then have a right-click context menu that allows one to edit entity settings.
// (location, rotation, scale, turn on/off collision, change mesh, delete)
// also have default actions like spawn object (selects based on entity type), save scene, scene settings (like lightning?)

//to make this work, have all types of objects have a pointer to 

struct special_ingame_object {


    struct object* location;
}

struct camera {

    float fov;
    float width;
    float height;
    float near;
    float far;


    mat4_t proj;
    mat4_t view;

};

struct player {
    
    vec3_t pos; 
    vec3_t vel;

    float yaw;
    float pitch;
    float radius;

    float has_dashed;
    float can_jump;

    struct camera cam;
    struct timer t;

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
    float zoom         = 0.0f;

    int restart = 0;
    int quit    = 0;
};

void update_player(struct player_input& i, struct player& p) {



    last_frame_pos = player.pos;


    // should we rotate camera first? not sure what impact is has on gameplay...


    // update move based on either yaw or pitch.
    acc.x = cosf(pitch); //something like this.
    acc.z = sinf(pitch);


    acc += i.move_x
    vel += acc;
    pos += vel;

    // https://www.youtube.com/watch?v=j_bTClbcCao
    // bilinear interp
    // check collision with terrain, just z compared with heightmap to make super mario type of level.
    const grid_size = 1.0f;

    int x;

    // check for collisions with static objects
    // for (int i = 0; i < static_objects.length; i++) {
        // get closest point on AABB
        // if dist to point from player is < player radius, collision occured.
        //    resolve collision. Should we use point? as a start we can just check each axis movement and remove the one that caused collision.
        //    here we should also consider allowing stairs, by just moving upwards. for this we might need to have preserved last frame's position and compare it.

    //}
    
    // check for collisions with moving objects, similar to above but we should resolve collisions a bit different.

    // check for collisions with enemies, also similar but resolution is widely different.

    // check input for player, and for example if we should punch something in front of us. 
    // if so, generate a small collision box sphere in front of us, and do another wave of collision checks with it.
    // these we can do with only enemies of perhaps even static objects, to recreate mario wall punch.
    // this box only checks the collision for this frame. 

}


// all structs have both their gameplay values and their rendering values, 
// should usually just be uniforms though.
struct moving_enemy {

    vec3_t base_pos;
    vec3_t current_pos;
    vec3_t target_pos;
    float radius;

    int chase_player;
    float speed;
};


// Maybe make enemies move around in 2d, 
// but just update their 3d pos to the ground by using raycast?
void update_moving_enemies() {

    for all:

        if (chase_player) {
            // update target_pos
            // if target_pos too far away
                // chase_player = 0;
                // target_pos = base_pos;

        }


}

void update_spline_enemies() {

    for all:
        move along spline;
}

struct menu_input {

    int move    = 0;
    int action  = 0;
    int mouse_x = 0;
    int mouse_y = 0;
    int mouse_active = 0;
}





int active_players = 1;






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

std::map<std::string, struct renderable> loadAllObjs() {

    std::map<std::string, struct renderable> loadedObjs;
    std::map<std::string, struct AABB> loadedAABBS;

    if (!std::filesystem::exists(textureDir) || !std::filesystem::is_directory(textureDir)) {
        log("obj directory not found");
        SDL_Quit();
    }

    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(textureDir)) {

        if (!entry.is_regular_file()) { continue; }

        std::ifstream file(entry.path());

        if (!file.is_open()) { log("failed to open " + entry.path()); continue; }

        std::string line;

        std::string fileName = entry.path().stem().string();

        std::vector<float> vertexes;
        std::vector<float> uvs;
        std::vector<float> normals;

        std::set<std::tuple<int, int, int> keys;
        std::vector<int> indexes;

        struct AABB col;
        col.min = INT_MAX;
        col.max = INT_MIN;

        while(std::getline(file, line)) {

            std::stringstream ss(line);
            std::vector<std::string> tokens;
            std::string token;
            while (ss >> token) { tokens.push_back(token); }

            if (tokens[0] == "v" && tokens.length == 4) { // conv to helper func

                vertexes.push_back(tokens[1]); 
                vertexes.push_back(tokens[2]); 
                vertexes.push_back(tokens[3]); 

                // For AABB
                // Extract vector
                // Compare each point of vertex w each point of min and max
                // if lower, update min, if higher, update max;
            }

            if (tokens[0] == "vt" && tokens.length == 3) { 

                uvs.push_back(tokens[1]); 
                uvs.push_back(tokens[2]);  
            }

            if (tokens[0] == "vn" && tokens.length == 4) { 

                normals.push_back(tokens[1]); 
                normals.push_back(tokens[2]); 
                normals.push_back(tokens[3]); 
            }

            if (tokens[0] == "f" && tokens.length == 4) { 

                for (int i = 1; i <= 3; i++) {

                    std::tuple<int, int, int> index = extractIndices(tokens[i]);
                    if (keys.find(index)) { indexes.push_back(found_key); } 
                    else                  { keys.push_back(index); indexes.push_back(keys.length); }
                }
            }
        }

        col.mid = (col.max + col.min) / 2;

        unsigned int VBO, EBO, VAO;
        glGenVertexArrays(1, &VAO); 
        glGenBuffers     (1, &VBO);      
        glGenBuffers     (1, &EBO);   

        glBindVertexArray(VAO); 
        glBindBuffer     (GL_ARRAY_BUFFER, VBO);
        glBindBuffer     (GL_ELEMENT_ARRAY_BUFFER, EBO);  

        glBufferData(GL_ARRAY_BUFFER,         sizeof(vertices), vertices, GL_STATIC_DRAW); // unique vertex data
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices),  indices,  GL_STATIC_DRAW); // indices

        // pos, dim, type, normalized, stride, offset
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) 0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (3 * sizeof(float)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (6 * sizeof(float)));

        glBindVertexArray(0);

        struct renderable r;
        r.VAO = VAO;
        r.VBO = VBO;
        r.EBO = EBO;
        r.size = keys.length;

        loadedObjs[fileName] = r;

    }

    return loadedObjs;
}

// Out game will max have 50 textures, so it makes sense to just load all of them at startup and leave them on the GPU
std::map<std::string, GLuint> loadAllTextures() {

    std::map<std::string, GLuint> loadedTextures;
    
    if (!std::filesystem::exists(textureDir) || !std::filesystem::is_directory(textureDir)) {
        log("texture directory not found");
        SDL_Quit();
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


void setupShaderPrograms() {

    for (auto s : shaderPrograms) {

        struct shader_program p;
        std::string name = s.first;
        p.vs   = s.second[0];
        p.fs   = s.second[1];

        p.pos = glCreateProgram();
        glAttachShader(p.pos, shaders[p.vs].pos);
        glAttachShader(p.pos, shaders[p.fs].pos);
        glLinkProgram(p.pos);

        int success; 
        char infoLog[512];
        glGetProgramiv(p.pos, GL_LINK_STATUS, &success);

        if (!success) { 
            log("invalid shader link - " + name);
            glGetProgramInfoLog(p.pos, 512, NULL, infoLog);
            SDL_Quit();
        } 

        update_uniforms(p.uniform_locations);

    }
}

void update_uniforms(std::map<std::string, GLuint> &uniform_locations) {

    uniform_locations.clear();

    GLint uniform_count = 0;
    glGetProgramiv(p.pos, GL_ACTIVE_UNIFORMS, &uniform_count);

    char buff[512];
    char* name = buff;

    for (int i = 0; i < uniform_count; i++) {

        GLint size;   // make use of this, incase we use array uniforms.    
        GLenum type;  // todo make use of this
        GLsizei length;

        glGetActiveUniform(p.pos, i, 512, &length, &size, &type, name);

        GLint location = glGetUniformLocation(p.pos, name);
        std::string str(name);
        uniform_locations[str] = location;
    }
}


void checkShaderPrograms() {

    for (auto p : programs) {

        struct shader_program &sp = p.second;
        struct shader& vs = shaders[sp.vs];
        struct shader& fs = shaders[sp.fs];

        if (vs.dirty || fs.dirty) {

            GLuint temp_pos = glCreateProgram();
            glAttachShader(temp_pos, shaders[p.vs].pos);
            glAttachShader(temp_pos, shaders[p.fs].pos);
            glLinkProgram(temp_pos);

            int success; 
            char infoLog[512];
            glGetProgramiv(temp_pos, GL_LINK_STATUS, &success);

            if (!success) { 

                log("hot linking failed " + p.first);
                glDeleteProgram(temp_pos);

            } else {

                glDeleteProgram(sp.pos);
                sp.pos = temp_pos;
                update_uniforms(sp.uniform_locations);

            }

        }

    }

}

void cleanShaders() { for (struct shader& s : shaders) { s.dirty = 0; } }
void loadShaders() {
 
    if (!std::filesystem::exists(shaderDir) || !std::filesystem::is_directory(shaderDir)) {
        log("texture directory not found");
        SDL_Quit();
    }

    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(shaderDir)) {

        if (!entry.is_regular_file()) { continue; }

        struct shader s;

        s.path = entry.path().string();
        s.name = entry.path().stem().string();
        std::ifstream ifs(s.path);

        s.last_source.assign((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
        if      (s.path[s.path.length() - 2] == "v") { s.type == GL_VERTEX_SHADER;   }
        else if (s.path[s.path.length() - 2] == "f") { s.type == GL_FRAGMENT_SHADER; }
        else { log("invalid shader exists"); continue; }
        s.dirty = false;
        s.last_change = std::time(nullptr);
        s.type = ?
        s.pos = compileShader(s.source, s.type);
          
        shaders[s.name] = s;
    }

}

void recompileShader(struct shader& s) {


    unsigned int new_pos = glCreateShader(s.type);
    std::ifstream ifs(s.path);
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    glShaderSource(new_pos, 1, content.c_str(), NULL);
    glCompileShader(new_pos);

    int success; 
    char infoLog[512];
    glGetShaderiv(shaderLoc, GL_COMPILE_STATUS, &success);

    if (!success) { 
        glDeleteShader(new_pos);
        log("invalid shader recompile");
    } else {
        glDeleteShader(s.pos); //delete old shader
        s.dirty = true;
        s.source = content;
        s.pos = new_pos;
        
    }

    s.last_change = std::time(nullptr); // we do this for both cases so that we dont spam fail for broken shaders

}

void checkShaders() {

    for (auto& s : shaders) {

        struct stat file_info;
        stat(s.source.c_str(), &file_info);
        if (file_info.st_mtime > s.last_change) {
            recompileShader(s);
        }
    }
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
        std::cout << "ERROR - Shader " << source << " compilation failed \n" << infoLog << std::endl;
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

// Movement of character can just be done 

void update_player_view(struct player& p) {

    // Y is Up.
    // The larger the pitch is, the closer to X,Z we will get.
    vec3_t camera_pos = vec3( cosf(radians(p.yaw)) * cosf(radians(p.pitch)),
                        sinf(radians(p.pitch)),
                        sinf(radians(p.yaw)) * cosf(radians(p.pitch)))

    vec3_t to   = vec3(p.pos.x, p.pos.y + 5, p.pos.z);
    vec3_t up   = vec3(0.0f, 1.0f, 0.0f);
    p.cam.view  = m4_look_at(camera_pos, to, up);

}

void setupPlayers(int count) {

    for (int i = 0; i < count; i++) {

        struct player p;
        p.pos        = vec3(level.startpos.x + i * 5, level.startpos.y, level.startpos.z);
        p.vel        = vec3(0,0,0);
        p.has_dashed = 0;
        p.can_jump   = 0;

        p.cam.fov    = 90.0f;
        p.cam.width  = screen_width / count;
        p.cam.height = screen_height / count;
        p.cam.near   = 0.1f;
        p.cam.far    = 1000.0f;
        p.cam.proj   = m4_perspective(p.cam.fov, p.cam.width / p.cam.height, near, far);

        update_player_view(p);

        reset_timer(p.t);

        players.push_back(p);

    }
}

void finishLevel(int player_id) {



}


int loadGoal(std::ifstream &level_file) {

    std::string goal_line;
    std::getline(level_file, goal_line);
    std::stringstream goal_ss(goal_line);

    if (goal_ss >> goal_x >> goal_y >> goal_z) { 
        return 0;
    } else {
        log("error loading goal");
        return -1;
    }
}

int loadStart(std::ifstream &level_file) {
    //prob identical to above...
}

int loadStatics(std::ifstream &level_file) {

    std::string line;
    while(std::getline(level_file, line)) {

        std::stringstream ss(line);
        struct static_obj o;
        if (!(ss >> name >> pos.x >> pos.y >> pos.z >> scale.x >> scale.y >> scale.z >> col)) { 
            log("error loading static object");
            return -1;
        } 

        static_vector.push_back(o);
    }

}



void loadLevel(int level) {

    log("attempting to read level " + std::to_string(level));
    active_level.vectors.clear(); //fix to more concrete.

    std::string level_path   = dataDir    + "level" + std::to_string(level) + ".txt";
    std::string terrain_path = terrainDir + "level" + std::to_string(level) + ".png";
    std::ifstream level_file(level_path);

    std::string line;
    int ret = 0;
    while(std::getline(level_file, line)) {

        if      (line.find("START:")    != std::string::npos) { ret = loadGoal    (level_file); }
        else if (line.find("GOAL:")     != std::string::npos) { ret = loadStart   (level_file); }
        else if (line.find("STATIC:")   != std::string::npos) { ret = loadStatics (level_file); }
        else if (line.find("MOVEABLE:") != std::string::npos) { ret = loadDynamics(level_file); }

        if (ret == -1) { log("loading map failed - abort"); SDL_Quit(); } // todo - go back to menu again...
    }

    // Load terrain!!!

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

    

    SDL_GetWindowSize(window, &screen_width, &screen_height);
    glViewport(0, 0, screen_width, screen_widht);
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f); 

    SDL_GL_SetSwapInterval(1); // vsync on


    SDL_Gamepad* activeGamepad = getGamepad();
    if (activeGamepad != nullptr) { log("Gamepad connected at startup - splitscreen available"); }

    bool quit = false;
    SDL_Event event;

    initializeFontAtlas();
    initializeTexts();
    // initialize all text objects that are needed for the game.
    loadAllTextures();
    loadAllObjs();

    loadShaders();
    loadShaderPrograms();

    unsigned int lastFrame = SDL_GetTicksMS();

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
                    case SDLK_UP:    { m_input.move   =  1; mouse_active = 0; break;  }
                    case SDLK_DOWN:  { m_input.move   = -1; mouse_active = 0; break;  }
                    case SDLK_SPACE:
                    case SDLK_ENTER: { m_input.action =  1; break; }
                    default: { break; } }
                }

                if (event.type == SDL_EVENT_MOUSE_MOTION) {
                    m_input.mouse_x = event.mmotion.x;
                    m_input.mouse_y = event.mmotion.y; 
                    mouse_active = 1;
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

        checkShaders();
        checkShaderPrograms();
        cleanShaders();

        // -------- LOGIC ----------- //


        if (gamemode == GameMode::MENU) {



            if (m_input.mouse_active) {
                int inside_borders = ( m_input.x > 100 && m_input.x < width - 100 && 
                                       m_input.y > 200 && m_input.y < height - 200 );
                if (inside_borders) {
                    active_menu_item = (m_input.y - 200) / 100; // this breaks if we change window size
                }
            } else {
                active_menu_item += m_input.move;  
            }

            if (m_input.press) {

                switch (active_menu_item) {
                case MenuItem::PLAYLEVEL1:  { loadLevel(1); setupPlayers(active_players); gamemode = GameMode::GAME; break; }
                case MenuItem::PLAYLEVEL2:  { loadLevel(2); setupPlayers(active_players); gamemode = GameMode::GAME; break; }
                case MenuItem::PLAYLEVEL3:  { loadLevel(3); setupPlayers(active_players); gamemode = GameMode::GAME; break; }
                case MenuItem::SPLITSCREEN: { active_players = active_players == 1 ? 2 : 1; break; }
                case MenuItem::QUIT:        { quit = true; break; } }
            }
        
        } else if (gamemode == GameMode::GAME) {

            for (int i = 0; i < active_players; i++) {

                update_player(p_input[i], players[i]); 

            }

            // update timer(s)

            // update player(s) (does collision checks)

        } else if (gamemode == GameMode::EDITOR) {

        }
        // nothing so far.. we are poor on logic.



        // --------------- RENDER --------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (gamemode == GameMode::MENU) {
            // render some type of background mby?
                // if so setup 3d pipeline etc.

            // setup everything for text rendering
            // is it anything more than glUseProgram???
            color colors[X] = { RED };
            colors[active_menu_item] = GREEN;
            std::vector<std::string> menu_entries = { "X", "Y", "Z"}
            // glUseProgram(programs["text2d"].pos);
            for (int i = 0; i < menu_entries.length(); i++) {
                color c = red;
                if (i == selected_index) { c = green; }
                drawtext_2d(menu_entries[i], x, y * i, size, c);
            }
            // drawText2d("blabla")
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
                //render 3d text
                // setup 3d text shader
                // for (3d_text t : 3d_texts) { drawtext_3d(t); }

                //render any ui stuff specific to a player
                //glDisable(GL_DEPTH_TEST);
                //glDisable(GL_CULL_FACE);
                //ui stuff
                //render timer
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

        } else if (gamemode == GameMode::Editor) {

        }

        // swap active render buffr.
        SDL_GL_SwapWindow(window);



        unsigned int currentFrame = SDL_GetTicksMS();
        deltaTime = 1.0f / (float)(currentFrame - lastframe); //TODO check calc.
        lastFrame = currentFrame;

    }

    // pointless cleanup... but avoids valgrind errors.
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}



