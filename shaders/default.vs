#version 460 core
layout (location = 0) in vec3 aPos; 
layout (location = 1) in vec3 aNormal; 
layout (location = 2) in vec2 aUV; 

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
  
out vec3 normal;
out vec2 uv;

void main() {

    //gl_Position = vec4(aPos * 0.1, 1);
    gl_Position = projection * view * model * vec4(aPos, 1.0); 
    normal = aNormal;
    uv = aUV;
    
}