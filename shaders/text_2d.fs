#version 460 core
in vec2 UV;
layout (location = 0) out vec4 fragColor; // Writes to framebuffers color.

layout (binding = 0) uniform sampler2D textAtlas; //hardcode to tex_unit 0
uniform vec3 color;

in vec3 testcolor;

void main() {


    float alpha   = texture(textAtlas, UV).r;  
    //vec4 c   = texture(textAtlas, UV); 
    //fragColor = c;
    //fragColor   = vec4(1, 0, 0, 1);
    fragColor   = vec4(color, alpha);

}