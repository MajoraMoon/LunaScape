#version 410 core
in vec2 TexCoord;   // Von Vertex Shader übergebene Texturkoordinaten
out vec4 FragColor; // Endgültige Fragmentfarbe

uniform sampler2D videoTexture; // Unser Video-Textur-Sampler

void main() {
  // Lese die Farbe aus der Textur und gib sie aus.
  FragColor = texture(videoTexture, TexCoord);
}
