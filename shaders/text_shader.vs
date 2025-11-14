#version 330 core
layout (location = 0) in vec2 vertexPos; //avoids glGetAttribLocation
layout (location = 1) in vec2 vertexUV;

out vec2 UV;

//pot add in uniforms like view/proj matrix

void main() {
    gl_Position = projection * vec4(vertexPos, 0.0, 1.0);
    UV = vertexUV;
}