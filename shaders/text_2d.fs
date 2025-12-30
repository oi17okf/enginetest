#version 460 core
in vec2 UV;
layout (location = 0) out vec4 fragColor; // Writes to framebuffers color.

layout (binding = 0) uniform sampler2D textAtlas; //hardcode to tex_unit 0
uniform vec3 color;

void main() {

    float alpha = texture(textAtlas, UV).r;  
    fragColor   = vec4(color, alpha);
}