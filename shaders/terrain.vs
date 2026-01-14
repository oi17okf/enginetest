#version 460 core
layout (location = 0) in vec3 vertexPos; //avoids glGetAttribLocation
layout (location = 1) in vec3 vertexColor; // note: we can skip locations

out vec3 color;

uniform mat4 projection;
uniform mat4 view;

void main () {
	
	color = vertexColor;
	float b = 0.001 * view[0][0];
	float c = 0.001* projection[0][0];
	gl_Position = projection * view * vec4(vertexPos, 1.0);
	//gl_Position = vec4(vertexPos, 1.0 + c + b);

}