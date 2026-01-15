#version 460 core
layout (location = 0) out vec4 fragColor;

in vec3 color;

void main () {
	
	fragColor = vec4(color.x / 255, color.y / 255, color.z / 255, 1.0);
}