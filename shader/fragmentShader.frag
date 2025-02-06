#version 410 core
in vec2 TexCoord;   // from vertex shader given coordinates
out vec4 FragColor; // frament colors

uniform sampler2D videoTexture; // video texture sampler

void main() {
  // reads the color from the texture and outputs it
  FragColor = texture(videoTexture, TexCoord);
}
