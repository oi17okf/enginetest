#version 460 core
layout (location = 0) in vec3 aPos; 
layout (location = 1) in vec3 aNormal; 
layout (location = 2) in vec3 aUV; 

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
  
out vec4 vertexColor; 

void main() {

    gl_Position = vec4(aPos, 1.0); 
    
}