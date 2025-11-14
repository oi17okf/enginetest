#version 330 core
in vec2 UV;
layout (location = 0) out vec4 color; // Writes to framebuffers color.

layout (binding = 0) uniform sampler2D textAtlas; //hardcode to tex_unit 0
//uniform vec3 textColor;

void main() {
    // Sample the single-channel texture (often GL_RED or GL_ALPHA)
    float alpha = texture(textAtlas, UV).r; 
    
    // Output the final color with the sampled alpha
    color = vec4(1, 0, 0, alpha);
}