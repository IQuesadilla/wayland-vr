#version 450

// Vertex attributes
layout(location = 0) in uint VertexIndex;

// Output to fragment shader
layout(location = 0) out vec4 Color;

void main() {
  vec2 pos;
  if (VertexIndex == 0) {
    pos = vec2(-1.0, -1.0);
    Color = vec4(1.0, 0.0, 0.0, 1.0);
  } else if (VertexIndex == 1) {
    pos = vec2(1.0, -1.0);
    Color = vec4(0.0, 1.0, 0.0, 1.0); // Green
  } else if (VertexIndex == 2) {
    pos = vec2(0.0, 1.0);
    Color = vec4(0.0, 0.0, 1.0, 1.0); // Blue
  }
  gl_Position = vec4(pos, 0.0, 1.0);
}

