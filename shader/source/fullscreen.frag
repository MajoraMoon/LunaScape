#version 450
layout(binding = 0) uniform sampler2D myTexture;
layout(location = 0) in vec2 TexCoord;
layout(location = 0) out vec4 FragColor;
void main() { FragColor = texture(myTexture, TexCoord); }
