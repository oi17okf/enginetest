#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <set>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>



#define MATH_3D_IMPLEMENTATION
extern "C" {
    #include "math_3d.h"
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "glad/glad.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>


const std::string textureDir   = "textures\\";
const std::string objectDir    = "objects\\";
const std::string terrainDir   = "saves\\terrains\\";
const std::string shaderDir    = "shaders\\";
const std::string detailsDir   = "saves\\details\\";
const std::string timesDir   = "saves\\times\\";
const std::string dataDir      = "data\\";
const std::string atlasLoc     = "fontatlas.png";
const std::string levelDataloc = "data\\leveltimes.txt";


static inline void log(const char* s) { std::cout << s << std::endl; }
static inline void log(std::string s) { std::cout << s << std::endl; }

#include "shaders.h"

static inline radians(float deg) { return deg * M_PI / 180.0f; }



// MUST Before monday.
// Load in level w terrain & static objects
// Have a floating camera that can move around in the level.

// Ideally before sunday.
// Change camera to player that can move and has collisions on terrain & objects. 
// Make it start at start and end level if it touches end.

// make it so a player (sphere) can walk across terrain on a level w a camera that can be moved by the mouse.
// integrate start & goal along w a timer / highscore. (if difficult, ignore visualizing timer)
// add in static objects w collisions
// integrate splitscreen
// make player controls a bit fun along w adding legs / simple animations
// add in moving platforms, enemies, bouncers along w something interactable
// update plan after this


// frustrum culling?

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


#define DEFAULT_WINDOW_WIDTH  640
#define DEFAULT_WINDOW_HEIGHT 480


struct text_info  { int pos; 
                    int len; };
struct renderable { unsigned int VAO, VBO, EBO; 
                    int size; };
struct AABB       { vec3_t min, max, mid; };
struct texture    { int slot, id; };

struct render_globals {

    GLuint fontAtlasTextureID = 0;
    GLuint textVAO, textVBO, textEBO;

    GLuint terrainVAO, terrainVBO, terrainEBO;
    int terrain_indices;

    std::map<std::string, struct renderable>     renderables;
    std::map<std::string, struct text_info>      text_infos;
    std::map<std::string, struct shader>         shaders;
    std::map<std::string, struct shader_program> programs;
    std::map<std::string, struct texture>        textures;


};

struct globals {

    int window_width  = 0;
    int window_height = 0;

    float        delta_time           = 0.0f;
    int          fps                  = 0;
    unsigned int last_frame_timestamp = 0;

    std::vector<std::string> menu_entries = { "PLAYLEVEL1", "EDITOR", "SPLITSCREEN", "QUIT" };

    SDL_Gamepad*  gamepad;
    SDL_Window*   window;
    SDL_GLContext glContext;

    std::map<std::string, struct AABB> collisions; // global collisions.

    struct render_globals r;
};

struct globals g;

struct static_object {

    std::string mesh_name; // use this to find what to render.
    std::string texture_name; // what texture to use when rendering
    vec3_t pos;
    vec3_t scale;

    int has_collision;

};

// we could calc color from height...
struct terrain {

    int size;

    // these are both bpp = 4, to make writing / reading simpler.
    std::vector<unsigned char> colors; // Allows for simple editing
    std::vector<unsigned char> height; // Same but also used for collision

    int dirty = 0; //could be used as index, if we only change 1 height vert at a time.
    // Otherwise make a vector of changed vertices.
    std::string name;

};

struct level_info {

    std::string visual_name;
    std::string file_name;
    float global_best_time;
    float local_best_time;
    float current_best_time;
    float gold_time;
    float silver_time;
    float bronze_time;

    struct terrain t;

    vec3_t start_pos;
    vec3_t goal_pos;

    std::vector<struct static_object> statics;

};

enum class GameMode { INIT, MENU, PLAY, EDIT, QUIT };

void clamp(int &val, int min, int max, int wrap) {

    if (val < min && wrap) { val = max; }
    if (val < min)         { val = min; }
    if (val > max && wrap) { val = min; }
    if (val > max)         { val = max; }

}

void clamp(float &val, int min, int max, int wrap) {

    if (val < min && wrap) { val = max; }
    if (val < min)         { val = min; }
    if (val > max && wrap) { val = min; }
    if (val > max)         { val = max; }

}

static inline void update_shader_uniform(std::string shader, std::string uniform, int val) {

    try {
        unsigned int loc = g.r.programs.at(shader).uniform_locations.at(uniform);
        glUniform1i(loc, val);
    } catch (const std::out_of_range& e) {
        log("failed to update " + uniform + " in program " + shader);
        SDL_Quit();
    }
}

static inline void update_shader_uniform(std::string shader, std::string uniform, mat4_t val) {

    try {
        unsigned int loc = g.r.programs.at(shader).uniform_locations.at(uniform);
        glUniformMatrix4fv(loc, 1, GL_FALSE, (float*) &val);
    } catch (const std::out_of_range& e) {
        log("failed to update " + uniform + " in program " + shader);
        SDL_Quit();
    }
}

static inline void setActiveProgram(std::string program) {

    unsigned int p = g.r.programs[program].pos;
    glUseProgram(p);
    
}


// This can be updated to use SDL_GetPerformanceCounter() 
// && SDL_GetPerformanceFrequency() if needed.
void update_fps() {

    unsigned int current_frame = SDL_GetTicks();
    unsigned int ms_since_last_frame = (float)(current_frame - g.last_frame_timestamp);

    g.delta_time = ms_since_last_frame / 1000; 
    g.fps = 1.0f / g.delta_time;
    g.last_frame_timestamp = current_frame;
}

void limit_fps(int fps_cap) {

    float needed_delta = 1.0f / (float)fps_cap;

    if (g.delta_time < needed_delta) {

        SDL_Delay((needed_delta - g.delta_time) * 1000 );
    }
}



void initializeTimerRenderer() {


    // Create a dynamic VBO w XX:XX:XXX (9) quads and configure everything properly.

    // Then just have a functions that looks at a time value 
    // and changes the UV's so that the correct values are visualized properly.

}



void appendVertexData(std::string s, int pos, std::vector<float> &vertexes, std::vector<unsigned int> &indexes) {

    const int   ascii_offset = -32; // Space has ascii index 32, altas start with Space.
    const int   atlas_size   = 10;
    const float uv_partition = 1.0f / (float)atlas_size;

    for (int j = 0; j < s.size(); j++) {

        char c = s[j];
        int uv_y = 9 - (c + ascii_offset) / atlas_size;
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

        unsigned int offset = (unsigned int)j * 4 + pos * 4;
        std::vector<unsigned int>   new_indices  = { 0 + offset, 1 + offset, 2 + offset, 0 + offset, 2 + offset, 3 + offset};
        std::vector<float> new_vertexes = { pos_x_1, pos_y_2, uv_x_1, uv_y_2,   // bl
                                            pos_x_2, pos_y_2, uv_x_2, uv_y_2,   // br
                                            pos_x_2, pos_y_1, uv_x_2, uv_y_1,   // tr
                                            pos_x_1, pos_y_1, uv_x_1, uv_y_1 }; // tl


        vertexes.insert(vertexes.end(), new_vertexes.begin(), new_vertexes.end());
        indexes.insert(indexes.end(), new_indices.begin(),   new_indices.end());

    }
}
 
// poor name but HEY
// also early return but no police here
void refreshTexts(std::string s) {

    if (g.r.text_infos.find(s) != g.r.text_infos.end()) { return; }

    log("rebuilding texts...");

    std::vector<float> vertexes;
    std::vector<unsigned int>   indexes;

    int final_pos = 0;

    for (auto pair : g.r.text_infos) {

        std::string      name = pair.first;
        struct text_info info = pair.second;

        if (info.pos != final_pos) { log("error refreshTexts"); SDL_Quit(); }
        final_pos =+ info.len;

        appendVertexData(name, info.pos, vertexes, indexes);

    }

    appendVertexData(s, final_pos, vertexes, indexes);

    struct text_info info;
    info.pos = final_pos;
    info.len = s.size();

    g.r.text_infos[s] = info;

    // Talk about this... 
    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, g.r.textVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.r.textEBO);

    glBufferData(GL_ARRAY_BUFFER,         vertexes.size() * sizeof(float), vertexes.data(), GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size()  * sizeof(int),   indexes.data(),  GL_STATIC_DRAW);


}


void initializeTexts() {

    std::vector<std::string> game_strings {

        "PLAY", "SPLITSCREEN", "QUIT", "START", "GOAL", "ENEMY", "RETURN",
        "CREATE NEW LEVEL!", "TOGGLE PLAYERS", "EDIT", "REFRESH LEVELS",
        "LEVEL1", "LEVEL2", "LEVEL3"
    };

    glGenVertexArrays(1, &g.r.textVAO);
    glGenBuffers     (1, &g.r.textVBO);
    glGenBuffers     (1, &g.r.textEBO);

    std::vector<float> vertexes;
    std::vector<unsigned int>   indexes;
    int pos = 0; 

    for (int i = 0; i < game_strings.size(); i++) {

        std::string s = game_strings[i];
        log(s);

        appendVertexData(s, pos, vertexes, indexes);

        struct text_info info;
        info.pos = pos;
        info.len = s.size();

        pos += s.size();

        g.r.text_infos[s] = info;

    }

    std::cout << vertexes.size() << std::endl;
    std::cout << indexes.size() << std::endl;
    std::cout << pos << std::endl;

   
    glBindVertexArray(g.r.textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g.r.textVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.r.textEBO);

    glBufferData(GL_ARRAY_BUFFER,         vertexes.size() * sizeof(float),        vertexes.data(), GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size()  * sizeof(unsigned int), indexes.data(),  GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (2 * sizeof(float)));


    // Should we have this? I dont like this design...
    glBindVertexArray(0);
}


//here we assume we are already in proper text/2d mode/correct shader.
void drawtext_2d(std::string text, int x, int y, int size, vec3_t color) {

    refreshTexts(text);

    struct text_info t;
    try {
        t = g.r.text_infos.at(text);
    } catch (const std::out_of_range& e) {
        // TODO: Make this more dynamic, preferably edit buffer w new string.
        log("Could not find text " + text + " when drawing 2d - abort");
        SDL_Quit();
    }

    
    glActiveTexture(GL_TEXTURE0); 
    glBindTexture(GL_TEXTURE_2D, g.r.fontAtlasTextureID);

    GLint texLoc = glGetUniformLocation(g.r.programs.at("text2d").pos, "textAtlas");
    glUniform1i(texLoc, 0);
    try {
        unsigned int pos_loc = g.r.programs.at("text2d").uniform_locations.at("position");
        glUniform2f(pos_loc, x, y);
    } catch (const std::out_of_range& e) {
        log("failed to set pos");
        SDL_Quit();
    }

    try {
        unsigned int size_loc = g.r.programs.at("text2d").uniform_locations.at("size");
        glUniform1i(size_loc, size);
    } catch (const std::out_of_range& e) {
        log("failed to set size");
        SDL_Quit();
    }

    try {
        unsigned int color_loc = g.r.programs.at("text2d").uniform_locations.at("color");
        glUniform3fv(color_loc, 1, (const float*)&color);
    } catch (const std::out_of_range& e) {
        log("failed to set col");
        //SDL_Quit();
    }

    glBindVertexArray(g.r.textVAO); // This could be set outside, 
    // since we are already calling all draw2d at the same time.
    int num_indices = t.len * 6;
    size_t byte_offset = (size_t)t.pos * 6 * sizeof(unsigned int);
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, (void*)byte_offset);


}

struct text_3d {

    std::string text;
    vec3_t c;
    vec3_t pos;
    vec3_t rot;
    vec3_t scale;
};

/*
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

*/




void loadTerrain(struct level_info &l, std::string file_name) {

    int width;
    int height;
    int nr_channels;

    unsigned char* data;

    std::string terrain_color = terrainDir + file_name + "_color.png";
    data = stbi_load(terrain_color.c_str(), &width, &height, &nr_channels, STBI_rgb_alpha);
    l.t.colors.assign(data, data + (width * height * nr_channels));
    stbi_image_free(data);

    std::string terrain_height = terrainDir + file_name + "_height.png";
    data = stbi_load(terrain_height.c_str(), &width, &height, &nr_channels, STBI_rgb_alpha);
    l.t.height.assign(data, data + (width * height * nr_channels));
    stbi_image_free(data);

    l.t.size = width;
    // We should prob assert width/height/channels better but w.e
}


// Split into initialize / upload / refresh.
// upload just looks at the data and fills/replaces the enitire buffer without caring
// refresh looks at 'changed' vertices and swaps only those
// https://www.learnopengles.com/android-lesson-eight-an-introduction-to-index-buffer-objects-ibos/

void initTerrain() {

    glGenVertexArrays(1, &g.r.terrainVAO);
    glGenBuffers     (1, &g.r.terrainVBO);
    glGenBuffers     (1, &g.r.terrainEBO);

    glBindVertexArray(g.r.terrainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g.r.terrainVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.r.terrainEBO);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (3 * sizeof(float)));

    glBindVertexArray(0);
}

void uploadTerrain(struct level_info &l, int editMode) {

    // dangerous assumptions...
    int width  = l.t.size;
    int height = l.t.size;
    int bpp    = 4;

    std::vector<float> vertex;
    std::vector<int>   index;

    // While a visual map looks to represent a "tile",
    // it insteads represent the top left vertex of that tile.

    const float grid_size = 1.0f;

    for (int i = 0; i < height; i++) {

        for (int j = 0; j < width; j++) {

            //look at types here. Not sure if we index properly, given that we get uint8 to float.
            float height = (float)l.t.height[(i * width + j) * bpp];
            // we should also normalize height somewhat?

            // Just input data for VBO here.
            vertex.push_back(j *  grid_size); // X
            vertex.push_back(i * -grid_size); // Y, grow down.
            vertex.push_back(height);

            float r = (float)l.t.colors[(i * width + j) * bpp];
            float g = (float)l.t.colors[(i * width + j) * bpp + 1];
            float b = (float)l.t.colors[(i * width + j) * bpp + 2];

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

    glBindVertexArray(g.r.terrainVAO); // we could skip this if we assumed no vao is bound.
    glBindBuffer(GL_ARRAY_BUFFER, g.r.terrainVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.r.terrainEBO);

    GLenum buffer_type = editMode ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    glBufferData(GL_ARRAY_BUFFER,         vertex.size() * sizeof(float),         vertex.data(), buffer_type); // Change when we make terrain editable
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index.size()  * sizeof(unsigned int),  index.data(),  GL_STATIC_DRAW); // indices

    g.r.terrain_indices = index.size();

    glBindVertexArray(0);

}

void generateDefaultTerrain(std::string file_name) {

    const int width    = 512;
    const int height   = 512;
    const int channels = 4; 

    std::vector<unsigned char> colors(width * height * channels);
    std::vector<unsigned char> heights(width * height * channels);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            int i = (y * width + x) * channels;

            
            colors[i + 0] = (unsigned char)(x % 256); 
            colors[i + 1] = (unsigned char)(y % 256); 
            colors[i + 2] = 128;                      
            colors[i + 3] = 255;    

            heights[i + 0] = 128; 
            heights[i + 1] = 128; 
            heights[i + 2] = 128;                      
            heights[i + 3] = 255;                     
        }
    }

    stbi_write_png((terrainDir + file_name + "_color.png").c_str(),  width, height, channels, colors.data(),  width * channels);
    stbi_write_png((terrainDir + file_name + "_height.png").c_str(), width, height, channels, heights.data(), width * channels);
}


void saveTerrain() {

    
}

void checkTerrain(struct level_info &l, std::string file_name) {
    
    std::ifstream terrain_file(terrainDir + file_name + "_color.png");

    if (!terrain_file.good()) {

        generateDefaultTerrain(file_name);
    } else {

        terrain_file.close();
    }

    loadTerrain(l, file_name);

}




// for level editor stuff, just implement raycast on collision box,
// then have a right-click context menu that allows one to edit entity settings.
// (location, rotation, scale, turn on/off collision, change mesh, delete)
// also have default actions like spawn object (selects based on entity type), save scene, scene settings (like lightning?)

//to make this work, have all types of objects have a pointer to 

struct special_ingame_object {


    struct object* location;
};


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
    float timer;

};


void update_player(struct player_input& i, struct player& p) {



   // last_frame_pos = player.pos;


    // should we rotate camera first? not sure what impact is has on gameplay...


    // update move based on either yaw or pitch.
    //acc.x = cosf(pitch); //something like this.
    //acc.z = sinf(pitch);


    //acc += i.move_x
    //vel += acc;
    //pos += vel;

    // https://www.youtube.com/watch?v=j_bTClbcCao
    // bilinear interp
    // check collision with terrain, just z compared with heightmap to make super mario type of level.
    const int grid_size = 1.0f;

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

/* 
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

*/

// implement Draw Call Reordering 

// draw instances

// do basic terrain w custom collision




void initializeFontAtlas() {

    int width;
    int height;
    int nr_channels;
    unsigned char* data = stbi_load(atlasLoc.c_str(), &width, &height, &nr_channels, STBI_rgb_alpha);

    GLenum format = (nr_channels == 4) ? GL_RGBA : GL_RGB;

    if (nr_channels == 4) { log("Channels: " + std::to_string(nr_channels)); }
    std::cout << width << " " << height << std::endl;

    if (!data) {
        log("fk");
        SDL_Quit();
    }

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &g.r.fontAtlasTextureID);
    glBindTexture(GL_TEXTURE_2D, g.r.fontAtlasTextureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // These maybe should be changed to NEAREST for less blurryness.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);           


    stbi_image_free(data);
}

// assume shader program is set
void renderObj(std::string name) {

    struct renderable r = g.r.renderables[name];

    glBindVertexArray(r.VAO);

    glDrawElements(GL_TRIANGLES, r.size, GL_UNSIGNED_INT, (void*)0);

}

void renderStatic(struct static_object &obj) {


    // set pos uniform
    // set scale uniform
    // or just do model directly.

    struct texture t = g.r.textures[obj.texture_name];
    update_shader_uniform("default", "textureID", t.slot);

    renderObj(obj.mesh_name);
}

void loadAllObjs() {

    std::map<std::string, struct AABB> loadedAABBS;

    if (!std::filesystem::exists(objectDir) || !std::filesystem::is_directory(objectDir)) {
        log("obj directory not found");
        SDL_Quit();
    }

    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(objectDir)) {

        if (!entry.is_regular_file()) { continue; }

        std::ifstream file(entry.path());

        if (!file.is_open()) { log("failed to open " + entry.path().string()); continue; }

        std::string line;

        std::string name;

        std::vector<vec3_t> vertexes;
        std::vector<vec3_t> uvs;
        std::vector<vec3_t> normals;

        std::set<std::tuple<int, int, int>> found_keys;
        std::vector<int> indexes;
        std::vector<float> vbo_data;

        struct AABB col;
        col.min = vec3(INT_MAX, INT_MAX, INT_MAX);
        col.max = vec3(INT_MIN, INT_MIN, INT_MIN);

        // This is all a bit stupid... but i am tired...

        while(std::getline(file, line)) {

            std::stringstream ss(line);
            std::string linetype;

            if (!(ss >> linetype)) { continue; }

            if (linetype == "o") { ss >> name; }

            if (linetype == "v") { // This is horrible. So error prone...

                float vx, vy, vz = 0.0f;
                ss >> vx >> vy >> vz;

                col.min.x = vx < col.min.x ? vx : col.min.x;
                col.max.x = vx > col.max.x ? vx : col.max.x;

                col.min.y = vy < col.min.y ? vy : col.min.y;
                col.max.y = vy > col.max.y ? vy : col.max.y;

                col.min.z = vz < col.min.z ? vz : col.min.z;
                col.max.z = vz > col.max.z ? vz : col.max.z;

                vec3_t pos = vec3(vx, vy, vz);
                vertexes.push_back(pos);
      
            }

            if (linetype == "vt") { 

                float uvx, uvy = 0.0f;
                ss >> uvx >> uvy;
                vec3_t uv = vec3(uvx, uvy, 0);
                uvs.push_back(uv);  
            }

            if (linetype == "vn") { 

                float nx, ny, nz = 0.0f;
                ss >> nx >> ny >> nz;
                vec3_t normal = vec3(nx, ny, nz);
                normals.push_back(normal); 

            }

            if (linetype == "f") { 

                std::string faces[3];
                ss >> faces[0] >> faces[1] >> faces[2];

                for (int i = 0; i < 3; i++) {

                    std::stringstream sss(faces[i]);
                    char slash;
                    int v, t, n;
                    sss >> v >> slash >> t >> slash >> n;
                    std::tuple<int, int, int> index = std::make_tuple(v, t, n);

                    auto it = found_keys.find(index);

                    if (it != found_keys.end()) { 

                        int key_index = std::distance(found_keys.begin(), it);
                        indexes.push_back(key_index); 

                    } else { 

                        found_keys.insert(index); 
                        indexes.push_back(found_keys.size() - 1); 

                        vec3_t pos    = vertexes[v];
                        vec3_t uv     = uvs[t];
                        vec3_t normal = normals[n];

                        vbo_data.push_back(pos.x);
                        vbo_data.push_back(pos.y);
                        vbo_data.push_back(pos.z);

                        vbo_data.push_back(uv.x);
                        vbo_data.push_back(uv.y);

                        vbo_data.push_back(normal.x);
                        vbo_data.push_back(normal.y);
                        vbo_data.push_back(normal.z);
                    }
                }
            }
        }

        col.mid = vec3((col.max.x + col.min.x) / 2, (col.max.y + col.min.y) / 2, (col.max.z + col.min.z) / 2);

        unsigned int VBO, EBO, VAO;
        glGenVertexArrays(1, &VAO); 
        glGenBuffers     (1, &VBO);      
        glGenBuffers     (1, &EBO);   

        glBindVertexArray(VAO); 
        glBindBuffer     (GL_ARRAY_BUFFER, VBO);
        glBindBuffer     (GL_ELEMENT_ARRAY_BUFFER, EBO);  

        glBufferData(GL_ARRAY_BUFFER,         vbo_data.size() * sizeof(float),        vbo_data.data(), GL_STATIC_DRAW); // unique vertex data
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size()  * sizeof(unsigned int), indexes.data(),  GL_STATIC_DRAW); // indices

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        // pos, dim, type, normalized, stride, offset
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) 0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (3 * sizeof(float)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (6 * sizeof(float)));

        glBindVertexArray(0);

        struct renderable r;
        r.VAO = VAO;
        r.VBO = VBO;
        r.EBO = EBO;
        r.size = indexes.size();

        g.collisions[name]    = col;
        g.r.renderables[name] = r;

    }

}



// Out game will max have 16 textures, so it makes sense to just load all of them at startup and leave them on the GPU

void loadAllTextures() {

    
    if (!std::filesystem::exists(textureDir) || !std::filesystem::is_directory(textureDir)) {
        log("texture directory not found");
        SDL_Quit();
    }

    int count = 1;

    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(textureDir)) {

        if (!entry.is_regular_file()) { continue; }

        std::string filePath = entry.path().string();
        std::string textureName = entry.path().stem().string();

        struct texture t;
        t.slot = count;

        int width;
        int height;
        int nr_channels;
        unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &nr_channels, 0);
        
        GLenum format = (nr_channels == 4) ? GL_RGBA : GL_RGB;
        
        GLuint textureID;
        glGenTextures(1, &textureID);
        glActiveTexture(GL_TEXTURE0 + count);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);           
        glGenerateMipmap(GL_TEXTURE_2D);
        

        t.id = textureID;
        g.r.textures[textureName] = t;
        stbi_image_free(data);
 
    }

    glBindTexture(GL_TEXTURE_2D, 0);

}


SDL_Gamepad* getGamepad() {

    int count = 0;
    SDL_JoystickID* gamepads = SDL_GetGamepads(&count);

    if (gamepads == nullptr || count <= 0) { return nullptr; }

    SDL_Gamepad* gamepad = SDL_OpenGamepad(gamepads[0]);

    SDL_free(gamepads);

    return gamepad;
}

int check_default_events(SDL_Event& event) {

    int quit = 0;

    switch (event.type) {

        case SDL_EVENT_QUIT: { quit = 1; break; }
        case SDL_EVENT_WINDOW_RESIZED: {

            SDL_GetWindowSize(g.window, &g.window_width, &g.window_height); 
            glViewport(0, 0, g.window_width, g.window_height);
            break;
        }

        case SDL_EVENT_GAMEPAD_ADDED: {

            if (g.gamepad == nullptr) {

                SDL_JoystickID instance_id = event.gdevice.which;
                g.gamepad = SDL_OpenGamepad(instance_id);
                log("Controller connected");
            }

            break;
        }

        case SDL_EVENT_GAMEPAD_REMOVED: {

            if (g.gamepad != nullptr && event.gdevice.which == SDL_GetGamepadID(g.gamepad)) {

                SDL_CloseGamepad(g.gamepad);
                g.gamepad = nullptr;
                log("Controller disconnected");
            }

            break;
        }
    }

    return quit;
}

// Movement of character can just be done 

/* 
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

*/

void finishLevel(int player_id) {



}


int loadGoal(struct level_info &l, std::ifstream &level_file) {

    if (!(level_file >> l.goal_pos.x >> l.goal_pos.y >> l.goal_pos.z)) { 
       log("error loading goal");
       return -1;
    } 

    return 0;
}

int loadStart(struct level_info &l, std::ifstream &level_file) {

    if (!(level_file >> l.start_pos.x >> l.start_pos.y >> l.start_pos.z)) { 
       log("error loading start");
       return -1;
    } 
    //prob identical to above...
    return 0;
}

// ugly (unlike you, dear student)
int loadStatics(struct level_info &l, std::ifstream &level_file) {

    std::string line;
    while(std::getline(level_file, line)) {

        std::stringstream ss(line);
        std::string first_word = "";
        ss >> first_word;
        if (first_word == "END:") { break; }
        if (first_word == "")     { log("error loading static object"); return -1; }

        struct static_object o;
        o.mesh_name = first_word;
        if (!(ss >> o.texture_name >> o.pos.x >> o.pos.y >> o.pos.z >> o.scale.x >> o.scale.y >> o.scale.z >> o.has_collision)) { 
            log("error loading static object");
            return -1;
        } 
        
        l.statics.push_back(o);
    }

    return 0;

}


void loadLevelDetails(struct level_info &l, std::string file_name) {

    std::ifstream level_file(detailsDir + file_name + ".txt");

    std::string line;
    int ret = 0;
    while(std::getline(level_file, line)) {

        if      (line.find("START:")    != std::string::npos) { ret = loadGoal    (l, level_file); }
        else if (line.find("GOAL:")     != std::string::npos) { ret = loadStart   (l, level_file); }
        else if (line.find("STATIC:")   != std::string::npos) { ret = loadStatics (l, level_file); }
        //else if (line.find("MOVEABLE:") != std::string::npos) { ret = loadDynamics(level_file); } add later

        if (ret == -1) { log("loading map failed - abort"); SDL_Quit(); } // todo - go back to menu again...
    }

}

void generateDefaultLevelDetails(std::string file_name) {

    std::fstream file(detailsDir + file_name + ".txt", std::ios::out | std::ios::trunc);

    file << "START:" << std::endl; 
    file << "0 0 0"  << std::endl;

    file << "GOAL:"     << std::endl; 
    file << "100 100 0" << std::endl; 

    file << "STATIC:" << std::endl; 
    file << "END:"    << std::endl;

    // TODO ADD MORE LATER
}

void checkLevelDetails(struct level_info &l, std::string file_name) {

    std::ifstream level_details(detailsDir + file_name + ".txt");
    if (!level_details.good()) {

        generateDefaultLevelDetails(file_name);

    } else {
        level_details.close();
    }

    loadLevelDetails(l, file_name);
}


// here we could add some sanity checks.
void saveLevelTime(std::string level_name, float time) {

    std::string file_name = timesDir + level_name + ".txt";

    std::ofstream outFile(file_name, std::ios::trunc);
    outFile << time;

}


void refreshLevels(std::map<std::string, struct level_info> &levels) {

    levels.clear();

    std::ifstream level_data_file(levelDataloc);
    std::string line;
    int line_count = 0;
    std::getline(level_data_file, line); // Throw away first line.
    while (std::getline(level_data_file, line)) {

        std::stringstream ss(line);
        struct level_info l;
        l.global_best_time = -1.0f;

        if (!(ss >> l.visual_name >> l.file_name >> l.gold_time >> l.silver_time >> l.bronze_time)) { 
            log("error loading medal times on line: " + std::to_string(line_count));
            line_count++;
            continue;
        } 

        levels[l.file_name] = l;

        line_count++;

    }

    for (auto &pair : levels) {

        struct level_info &l = pair.second;
        std::string file_name = l.file_name;

        // Open saved file.
        std::fstream save_file(timesDir + l.file_name, std::ios::in);
        if (save_file) { save_file >> l.global_best_time; }

        // Open terrain
        checkTerrain(l, l.file_name);

        checkLevelDetails(l, l.file_name);
        // open rest.
    }


}

/* &&&&&&&&&&&&&&&&& MENU &&&&&&&&&&&&&&&&&&&&&&&& */

struct menu_input {

    int move    = 0;
    int action  = 0;
    int back    = 0;

    int mouse_x = 0;
    int mouse_y = 0;
    int action_mouse = 0;
    int mouse_active = 0;

    int quit = 0;
};

struct menu_input getMenuInput() {

    struct menu_input m_input = { 0 }; 

    SDL_Event event;

    while (SDL_PollEvent(&event)) {

        m_input.quit = check_default_events(event);

        if (event.type == SDL_EVENT_KEY_DOWN) {

            switch (event.key.key) {

                case SDLK_BACKSPACE:
                case SDLK_ESCAPE: { m_input.back = 1; break; }

                case SDLK_UP:     { m_input.move   =  1; m_input.mouse_active = 0; break;  }
                case SDLK_DOWN:   { m_input.move   = -1; m_input.mouse_active = 0; break;  }
                case SDLK_SPACE:
                case SDLK_RETURN:  { m_input.action =  1; break; }

                default: { break; } 
            }
        }

        if (event.type == SDL_EVENT_MOUSE_MOTION) {

            m_input.mouse_x = event.motion.x;
            m_input.mouse_y = event.motion.y; 
            m_input.mouse_active = 1;
        }  

        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {

            switch (event.button.button) {

                case SDL_BUTTON_LEFT:  { m_input.action = 1;  m_input.mouse_active = 1; break; }

                default: { break; } 
            }
        } 

    }

    return m_input;
}

struct menu_state {

    int selected_players = 1;
    int active_item      = 0;
    int edit_level       = 0;
    int play_level       = 0;

    struct camera ortho_cam;

    std::map<std::string, struct level_info> levels;

    std::vector<std::string> menu_items = { "PLAY", "TOGGLE PLAYERS", "EDIT", "REFRESH LEVELS", "QUIT" };
    int item_space = 100;

};





GameMode doMenuLogic(struct menu_state& state, struct menu_input i) {

    log("Yello");
    GameMode gamemode = GameMode::MENU;

    int menu_items_count = state.menu_items.size();
    if (state.edit_level) { menu_items_count = state.levels.size() + 2; }
    if (state.play_level) { menu_items_count = state.levels.size() + 1; }

    if (i.mouse_active) {

        int inside_borders = ( i.mouse_x > 100 && i.mouse_x < g.window_width - 100 && i.mouse_y > state.item_space && i.mouse_y < (menu_items_count + 1) * state.item_space );
        if (inside_borders) {

            state.active_item = (i.mouse_y - state.item_space) / state.item_space;
        }

    } else {

        state.active_item += i.move; 
        clamp(state.active_item, 0, menu_items_count, 0);
    }

    if (i.action || i.mouse_active && i.action_mouse) {
        log("ACTION");

        if (state.edit_level) {

            if (state.active_item == 0) { 

                //createNewLevel();
                gamemode = GameMode::EDIT; 

            } else if (state.active_item == menu_items_count - 1) {

                state.edit_level  = 0;
                state.active_item = 2;

            } else {

                //setupEditLevel(state.active_item - 1);
                gamemode = GameMode::EDIT;
            }

        } else if (state.play_level) {

            if (state.active_item == menu_items_count - 1) {

                state.play_level  = 0;
                state.active_item = 0;
            } else {

                //setupPlayLevel(state.active_item - 1);
                gamemode = GameMode::PLAY;
            }

        } else {

            if (state.menu_items[state.active_item] == "PLAY")           { log("hi"); state.play_level = 1; }
            if (state.menu_items[state.active_item] == "TOGGLE PLAYERS") { state.selected_players = state.selected_players == 1 ? 2 : 1; }
            if (state.menu_items[state.active_item] == "EDIT")           { state.edit_level = 1; }
            if (state.menu_items[state.active_item] == "REFRESH LEVELS") { /*refreshLevels(state); */ }
            if (state.menu_items[state.active_item] == "QUIT")           { log("quit press"); gamemode = GameMode::QUIT; }
        }
    }

    if (i.back) {

        if (state.play_level || state.edit_level) {

            state.play_level = 0;
            state.edit_level = 0;

        } else {

            gamemode = GameMode::QUIT;
        }
    }

    if (i.quit) { gamemode = GameMode::QUIT; }

    return gamemode;

}

void renderMenu(const struct menu_state &state) {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Potential to render 3D background here. But add that later if there is time...

    // Turn on correct shader program.
    setActiveProgram("text2d");

    // vec3_t color = vec3(0, 1, 0);
    for (int i = 0; i < state.menu_items.size(); i++) {

        vec3_t color = vec3(1, 1, 1);
        if (i == state.active_item && state.edit_level == 0 && state.play_level == 0) { 

            color = vec3(0, 1, 0); 
        }
        drawtext_2d(state.menu_items[i], state.item_space, state.item_space + state.item_space * i, 100, color);
    }
    

    //for (int i = 0; i < game_strings.size(); i++) {
        //drawtext_2d(game_strings[i], state.item_space, state.item_space + state.item_space * i, 100, color, i);
    //}

    if (state.edit_level) {

        std::string t = "CREATE NEW LEVEL!";
        vec3_t color = vec3(1, 1, 1);
        if (state.active_item == 0) { color = vec3(0, 1, 0); }
        //drawtext_2d(t, state.item_space * 2, state.item_space, 10, color);

        int i = 0;
        for (auto pair : state.levels) {
            i++;
            color = vec3(1, 1, 1);
            std::string s = pair.first;
            if (state.active_item == i) { color = vec3(0, 1, 0); }
            //drawtext_2d(s, state.item_space * 2, state.item_space + state.item_space * i, 10, color);

        }

        std::string b = "RETURN";
        color = vec3(1, 1, 1);
        if (state.active_item == state.levels.size() + 1) { color = vec3(0, 1, 0); }
        //drawtext_2d(b, state.item_space * 2, (state.levels.size() + 2) * state.item_space, 10, color);

    } else if (state.play_level) {

        int i = 0;
        for (auto pair : state.levels) {

            vec3_t color = vec3(1, 1, 1);
            std::string s = pair.first;
            if (state.active_item == i) { color = vec3(0, 1, 0); }
            //drawtext_2d(s, state.item_space * 2, state.item_space + state.item_space * i, 10, color);
            i++;

        }

        std::string b = "RETURN";
        vec3_t color = vec3(1, 1, 1);
        if (state.active_item == state.levels.size()) { color = vec3(0, 1, 0); }
        //drawtext_2d(b, state.item_space * 2, (state.levels.size() + 1) * state.item_space, 10, color);

    }



    SDL_GL_SwapWindow(g.window);
}


struct menu_result {

    int active_players = 1;
    struct level_info l;

};

GameMode runMenu(struct menu_result &res) {

    struct menu_state state;
    GameMode gamemode = GameMode::MENU;
    refreshLevels(state.levels);
    log("inside menu");

    while (gamemode == GameMode::MENU) {

        struct menu_input i = getMenuInput();
        log("after input");
        gamemode = doMenuLogic(state, i);
        
        log("after logic");
        update_fps();
        limit_fps(1);
        log("after fps");

        checkShaders(g.r.shaders);
        checkShaderPrograms(g.r.programs, g.r.shaders);
        cleanShaders(g.r.shaders);

        renderMenu(state);
        log("menu loop");

    }

    log("outside menu");
    // Convert important info from state to settings / world? But we also want to throw away stuff...
    //cleanup, if any.

    res.active_players = state.selected_players;
    //res.l = ???

    return gamemode;

}

// Add pause if time...
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
    int pause   = 0;
    int quit    = 0;
};

void getPlayInput(int active_players, struct player_input* inputs) {

    int c = active_players - 1; // What to call this idk

    SDL_Event event;

    while (SDL_PollEvent(&event)) {

        inputs[0].quit = check_default_events(event);

        if (event.type == SDL_EVENT_KEY_DOWN) {

            switch (event.key.key) {

                case SDLK_SPACE:  { inputs[0].jump    = 1; break; }
                case SDLK_LSHIFT: { inputs[0].dash    = 1; break; }
                case SDLK_R:      { inputs[0].restart = 1; break; }
                case SDLK_P:      { inputs[0].pause   = 1; break; }
                
                default: { break; }
            }
        }

        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {

            switch (event.button.button) {

                case SDL_BUTTON_LEFT:  { inputs[0].punch = 1; break; }
                case SDL_BUTTON_RIGHT: { inputs[0].punch = 1; break; }

                default: { break; } 
            }
        }

        if (event.type == SDL_EVENT_MOUSE_MOTION) {

            if (event.motion.xrel > 0.01f || event.motion.xrel < -0.01f) {
                inputs[0].rotate_cam_x = event.motion.xrel;
            }

            if (event.motion.yrel > 0.01f || event.motion.yrel < -0.01f) {
                inputs[0].rotate_cam_y = event.motion.yrel;
            }
        }

        if (event.type == SDL_EVENT_MOUSE_WHEEL) {

            inputs[0].zoom = event.wheel.y;
        }   

        if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {

            switch (event.gbutton.button) {

                case SDL_GAMEPAD_BUTTON_SOUTH:          { inputs[c].jump    = 1;  break; }
                case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:  { inputs[c].dash    = 1;  break; }
                case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: { inputs[c].punch   = 1;  break; }
                case SDL_GAMEPAD_BUTTON_NORTH:          { inputs[c].restart = 1;  break; }
                case SDL_GAMEPAD_BUTTON_BACK:           { inputs[c].quit    = 1;  break; }
                case SDL_GAMEPAD_BUTTON_START:          { inputs[c].pause   = 1;  break; }

                default: { break; } 
            }
        }                                    
    }

    const bool* keystate = SDL_GetKeyboardState(nullptr);

    if (keystate[SDL_SCANCODE_SPACE]) { inputs[0].floating  = 1; }
    if (keystate[SDL_SCANCODE_W])     { inputs[0].move_y   += 1; }
    if (keystate[SDL_SCANCODE_S])     { inputs[0].move_y   -= 1; }
    if (keystate[SDL_SCANCODE_A])     { inputs[0].move_x   += 1; }
    if (keystate[SDL_SCANCODE_D])     { inputs[0].move_x   -= 1; }

    // if splitscreen & controller active
    if (g.gamepad != nullptr) {

        if (SDL_GetGamepadButton(g.gamepad, SDL_GAMEPAD_BUTTON_SOUTH)) { inputs[c].floating = 1; }


        inputs[c].move_x       = SDL_GetGamepadAxis(g.gamepad, SDL_GAMEPAD_AXIS_LEFTX)  / 32767.0f;
        inputs[c].move_y       = SDL_GetGamepadAxis(g.gamepad, SDL_GAMEPAD_AXIS_LEFTY)  / 32767.0f;
        inputs[c].rotate_cam_x = SDL_GetGamepadAxis(g.gamepad, SDL_GAMEPAD_AXIS_RIGHTX) / 32767.0f * 10.0f;
        inputs[c].rotate_cam_y = SDL_GetGamepadAxis(g.gamepad, SDL_GAMEPAD_AXIS_RIGHTY) / 32767.0f * 10.0f;

        clamp(inputs[c].move_x, -1, 1, 0);
        clamp(inputs[c].move_y, -1, 1, 0);
        clamp(inputs[c].rotate_cam_x, -10, 10, 0);
        clamp(inputs[c].rotate_cam_y, -10, 10, 0);
    } 
    
}

struct play_state {

    vec3_t start_point;
    vec3_t goal_point;

    int active_players;

    //std::vector<struct 3d_text>         3d_texts;
    std::vector<struct static_object>   statics;
    //std::vector<struct moveable_object> moveables;
    //std::vector<struct stat_boost>      stat_boosts; 
    //std::vector<struct enemy>           enemies;

    struct terrain t;

};

GameMode doPlayLogic(int active_players, struct play_state &state, struct player_input* inputs) {

    return GameMode::PLAY;
}


void renderTerrain(unsigned int terrainVAO, int terrain_indices, struct camera c) {

    update_shader_uniform("terrain", "projection", c.proj);
    update_shader_uniform("terrain", "view",       c.view);

    glBindVertexArray(terrainVAO);

    glDrawElements(GL_TRIANGLE_STRIP, terrain_indices, GL_UNSIGNED_INT, (void*) 0);

}


void renderPlay(int active_players, const struct play_state &state) {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (int i = 0; i < active_players; i++) {

        int startx = i       / active_players * g.window_width;
        int starty = i       / active_players * g.window_height;
        int endx   = (i + 1) / active_players * g.window_width;
        int endy   = (i + 1) / active_players * g.window_height;
        glViewport(startx, starty, endx, endy);

        
        //render static platform objects
        //render enemies
        //render player
        //render 3d text
        // setup 3d text shader
        // for (3d_text t : 3d_texts) { drawtext_3d(t); }

        setActiveProgram("terrain");
        renderTerrain(g.r.terrainVAO, g.r.terrain_indices, g.camera);
        //set shaderprogram
        // call render terrain.

        //render any ui stuff specific to a player
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        //ui stuff
        //render timer
        glEnable(GL_DEPTH_TEST); 
        glEnable(GL_CULL_FACE);
    } 

    glViewport(0, 0, g.window_width, g.window_height);
    //render any ui stuff not specific to a player
    // renderAllTextUI()
    //glDisable(GL_DEPTH_TEST);
    //glDisable(GL_CULL_FACE);



    glEnable(GL_DEPTH_TEST); 
    glEnable(GL_CULL_FACE);  

    SDL_GL_SwapWindow(g.window);
}


// This could be used to move between editor and play mode in a clunky but doable way...
struct play_result {

    float new_highscore;
};

GameMode runGame(struct play_result &res, struct level_info level_info, int active_players) {

    GameMode gamemode = GameMode::PLAY;
    struct play_state state;
    state.active_players = active_players;
    // Fill state with values from level_info...

    while (gamemode == GameMode::PLAY) {

        struct player_input inputs[active_players];

        getPlayInput(active_players, inputs);
        
        gamemode = doPlayLogic(active_players, state, inputs);
        update_fps();
        limit_fps(120);
        
        renderPlay(active_players, state);

        // Should we also do this in all other states?
        checkShaders(g.r.shaders);
        checkShaderPrograms(g.r.programs, g.r.shaders);
        cleanShaders(g.r.shaders);

    }




    //fill res
    res.new_highscore = 1;

    return gamemode;

}

struct edit_state {
    float something;
};

struct edit_result {

    float something;
};


GameMode runEditor(struct edit_result &res, struct level_info level_info) {

    GameMode gamemode = GameMode::EDIT;
    struct edit_state state;
    //fill w values from level_info

    //while (gamemode == GameMode::EDIT) {

        // Input
        // Logic
        // Render (will share a lot from game_render)
        // Most things her will, so make sure to do Play w functions.
    //}


    return gamemode;

}


int main(int argc, char* argv[]) {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Turn on double buffering and request a 24-bit depth buffer
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    g.window = SDL_CreateWindow("Gnario 64", DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!g.window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    g.glContext = SDL_GL_CreateContext(g.window);
    if (!g.glContext) {
        std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(g.window);
        SDL_Quit();
        return 1;
    }

    if (SDL_GL_MakeCurrent(g.window, g.glContext) < 0) {
        std::cerr << "Failed to make OpenGL context current! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_GL_DestroyContext(g.glContext);
        SDL_DestroyWindow(g.window);
        SDL_Quit();
        return 1;
    }

    
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        SDL_GL_DestroyContext(g.glContext);
        SDL_DestroyWindow(g.window);
        SDL_Quit();
        return 1;
    }

    
    std::cout << "OpenGL Version: " << GLVersion.major << "." << GLVersion.minor << std::endl;
    std::cout << "Vendor: "         << (char*)glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: "       << (char*)glGetString(GL_RENDERER) << std::endl;

    

    SDL_GetWindowSize(g.window, &g.window_width, &g.window_height);
    glViewport(0, 0, g.window_width, g.window_height);
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f); 

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    SDL_GL_SetSwapInterval(1); // vsync on


    g.gamepad = getGamepad();
    if (g.gamepad != nullptr) { log("Gamepad connected at startup - splitscreen available"); }

    GameMode gamemode = GameMode::INIT;

    stbi_set_flip_vertically_on_load(true);

    initializeFontAtlas();
    initializeTexts();

    initTerrain();
    // initialize all text objects that are needed for the game.
    loadAllTextures();
    loadAllObjs();

    loadShaders(g.r.shaders);
    setupShaderPrograms(g.r.programs, g.r.shaders);

    gamemode = GameMode::MENU;

    g.last_frame_timestamp = SDL_GetTicks();

    struct menu_result menu_res;
    struct play_result play_res;
    struct edit_result edit_res;

    log("HI");

    while (gamemode != GameMode::QUIT) {

        log("main loop");

        switch (gamemode) {

            case GameMode::MENU: { gamemode = runMenu  (menu_res);                                      break; }
            case GameMode::PLAY: { gamemode = runGame  (play_res, menu_res.l, menu_res.active_players); break; }
            case GameMode::EDIT: { gamemode = runEditor(edit_res, menu_res.l);                          break; }

        }


    }

    // pointless cleanup... but avoids valgrind errors.
    log("QUIT");
    SDL_GL_DestroyContext(g.glContext);
    SDL_DestroyWindow(g.window);
    SDL_Quit();

    return 0; 
}