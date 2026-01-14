#version 460 core
out vec4 fragColor;

in vec3 normal;
in vec2 uv;
  
uniform sampler2D textureID;

void main() {

    vec4 something = texture(textureID, uv);
    fragColor = vec4(something.x, 1, 1, 1);
} 