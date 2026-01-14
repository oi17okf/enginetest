#version 460 core
layout (location = 0) in vec2 vertexPos; //avoids glGetAttribLocation
layout (location = 1) in vec2 vertexUV; // note: we can skip locations

out vec2 UV;

uniform vec2 position;
uniform int  size;

uniform mat4 ortho_cam;
out vec3 testcolor;



void main() {
    
    float bla = ortho_cam[0][0];
    testcolor = vec3(ortho_cam[0][0], ortho_cam[1][1], ortho_cam[2][2]);

    vec2 screen_pos = position + (vertexPos * size);
    vec2 scaled_vertex = vertexPos * size;
    vec2 offset_vertex = scaled_vertex + position + bla;
    vec2 normalized    = offset_vertex / 2000;

    //gl_Position = vec4(normalized, 0.0, 1.0);
    vec3 testpos = vec3(ortho_cam[0][0] * screen_pos.x, ortho_cam[1][1] * screen_pos.y, 0);
    //gl_Position = vec4(testpos, 1.0);
    gl_Position = ortho_cam * vec4(screen_pos, 0.0, 1.0);
    
    UV = vertexUV;
}