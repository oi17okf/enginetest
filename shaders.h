std::map<std::string, std::pair<std::string, std::string>> program_definitions = {

    { "default", std::make_pair("default.vs", "default.fs") },
    { "terrain", std::make_pair("terrain.vs", "terrain.fs") },
    { "text2d",  std::make_pair("text_2d.vs", "text_2d.fs") },
    { "text3d",  std::make_pair("text_3d.vs", "text_3d.fs") }
};

struct shader {

    GLenum      type; // GL_FRAGMENT_SHADER or GL_VERTEX_SHADER
    std::time_t last_change;
    bool        dirty;
    std::string path;
    std::string name;
    std::string source;
    GLuint      pos;

};

struct shader_program {

    GLuint pos = 0;
    std::string vs;
    std::string fs;
    std::map<std::string, GLuint> uniform_locations;  //uniform locations are achieved through opengl reflection.
    
};

unsigned int compileShader(std::string s, GLenum type) {

    char* source = (char*)s.c_str();
    unsigned int shaderLoc = glCreateShader(type);
    glShaderSource(shaderLoc, 1, &source, NULL);
    glCompileShader(shaderLoc);

    int success; 
    char infoLog[512];
    glGetShaderiv(shaderLoc, GL_COMPILE_STATUS, &success);

    if (!success) { 

        glGetShaderInfoLog(shaderLoc, 512, NULL, infoLog);
        std::cout << "ERROR - Shader " << source << " compilation failed \n" << infoLog << std::endl;
    }

    return shaderLoc;

}

void update_uniforms(struct shader_program &p) {

    p.uniform_locations.clear();

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
        p.uniform_locations[str] = location;
    }
}

void setupShaderPrograms(std::map<std::string, struct shader_program> &programs, std::map<std::string, struct shader> &shaders) {

    programs.clear();

    // Global defined at the top of the file.
    for (auto pair : program_definitions) {

        struct shader_program p;
        std::string name = pair.first;
        p.vs   = pair.second.first;
        p.fs   = pair.second.second;

        p.pos = glCreateProgram();
        glAttachShader(p.pos, shaders[p.vs].pos);
        glAttachShader(p.pos, shaders[p.fs].pos);
        glLinkProgram(p.pos);

        int success; 
        char infoLog[512];
        glGetProgramiv(p.pos, GL_LINK_STATUS, &success);

        if (!success) { 
            glGetProgramInfoLog(p.pos, 512, NULL, infoLog);
            log("invalid shader link - " + name + " reason: " + infoLog);
            std::cout << infoLog;
            SDL_Quit();
        } 

        update_uniforms(p);

        programs[name] = p;

    }
}

void checkShaderPrograms(std::map<std::string, struct shader_program> &programs, std::map<std::string, struct shader> &shaders) {

    for (auto &p : programs) {

        struct shader_program &sp = p.second;
        struct shader& vs = shaders[sp.vs];
        struct shader& fs = shaders[sp.fs];

        if (vs.dirty || fs.dirty) {

            GLuint temp_pos = glCreateProgram();
            glAttachShader(temp_pos, vs.pos);
            glAttachShader(temp_pos, fs.pos);
            glLinkProgram(temp_pos);

            int success; 
            char infoLog[512];
            glGetProgramiv(temp_pos, GL_LINK_STATUS, &success);

            if (!success) { 

                log("hot linking failed " + p.first);
                glDeleteProgram(temp_pos);

            } else {

                // Look into better solution?
                glUseProgram(0);
                glDeleteProgram(sp.pos);
                sp.pos = temp_pos;
                update_uniforms(sp);

            }

        }

    }

}

void cleanShaders(std::map<std::string, struct shader> &shaders) {

    for (auto &item : shaders) { 
        
        item.second.dirty = 0; 
    } 
}

void loadShaders(std::map<std::string, struct shader> &shaders) {

    shaders.clear();
 
    if (!std::filesystem::exists(shaderDir) || !std::filesystem::is_directory(shaderDir)) {
        log("AHH");
        SDL_Quit();
    }

    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(shaderDir)) {

        if (!entry.is_regular_file()) { continue; }

        struct shader s;

        s.path = entry.path().string();
        s.name = entry.path().stem().string();
        std::ifstream ifs(s.path);

        s.source.assign((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
        if      (s.path[s.path.length() - 2] == 'v') { s.type = GL_VERTEX_SHADER;   s.name += ".vs"; }
        else if (s.path[s.path.length() - 2] == 'f') { s.type = GL_FRAGMENT_SHADER; s.name += ".fs"; }
        else { continue; }
        s.dirty = false;
        s.last_change = std::time(nullptr);
        s.pos = compileShader(s.source, s.type);
          
        shaders[s.name] = s;
    }

}

void recompileShader(struct shader& s) {

    unsigned int new_pos = glCreateShader(s.type);
    std::ifstream ifs(s.path);
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    const char* shader_code_ptr = content.c_str();
    glShaderSource(new_pos, 1, &shader_code_ptr, NULL);
    glCompileShader(new_pos);

    int success; 
    char infoLog[512];
    glGetShaderiv(new_pos, GL_COMPILE_STATUS, &success);

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

void checkShaders(std::map<std::string, struct shader> &shaders) {

    for (auto& pair : shaders) {

        struct shader &s = pair.second;
        struct stat file_info;
        stat(s.path.c_str(), &file_info);
        if (file_info.st_mtime > s.last_change) {
            log("trying to recompile");
            recompileShader(s);
        }
    }
}




