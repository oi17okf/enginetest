#version 460 core
layout (location = 0) out vec4 fragColor;

in vec3 color;


int main () {
	
	fragColor(color, 1.0);
}