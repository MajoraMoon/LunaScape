#version 410 core
layout(location = 0) in vec2 aPos;      // Vertex-Position (XY)
layout(location = 1) in vec2 aTexCoord; // Texturkoordinaten (UV)

out vec2 TexCoord; // Übergibt die Texturkoordinaten an den Fragment Shader

void main() {
  // Setze die Position des Vertex in Clip-Space
  gl_Position = vec4(aPos, 0.0, 1.0);
  TexCoord = aTexCoord;
}
