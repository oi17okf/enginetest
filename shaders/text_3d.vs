#version 460 core
layout (location = 0) in vec2 vertexPos; //avoids glGetAttribLocation
layout (location = 1) in vec2 vertexUV; // note: we can skip locations

out vec2 UV;

uniform vec2 position;
uniform int  size;



void main() {
    
    // Size only multiplies local vertex positions.
    vec2 scaled_vertex = vertexPos * size;
    vec2 offset_vertex = scaled_vertex + position;

    gl_Position = vec4(offset_vertex, 0.0, 1.0);
    UV = vertexUV;
}