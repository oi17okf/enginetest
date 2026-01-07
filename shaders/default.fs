#version 460 core
out vec4 FragColor;
  
in vec4 vertexColor;

uniform sampler2D textureID;

void main()
{
    FragColor = vertexColor;
} 