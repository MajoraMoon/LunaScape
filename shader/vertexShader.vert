#version 410 core
layout(location = 0) in vec2 aPos;      // Vertex-Position (XY)
layout(location = 1) in vec2 aTexCoord; // texture-position (UV)

uniform mat4 transform; // tranform matrix

out vec2 TexCoord; // tells the fragment shader the coordinates of the texture
                   // position

void main() {
  // sets the positions of the vertex data and sets the texture-coordinates for
  // the fragment shader
  gl_Position = transform * vec4(aPos, 0.0, 1.0);
  TexCoord = aTexCoord;
}
