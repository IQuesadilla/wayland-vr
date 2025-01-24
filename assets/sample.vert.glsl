#version 450

// Vertex attributes
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;

// Output to fragment shader
layout(location = 0) out vec2 outUV;

// Uniforms for transformation matrices
layout(set = 1, binding = 0) uniform Transforms {
  mat4 mvp;
};
//layout(binding = 0) uniform Matrices {
  //mat4 model;
  //mat4 view;
  //mat4 projection;
//};

void main() {
  // Calculate the final position of the vertex
  // gl_Position = projection * view * model * vec4(aPos, 1.0);
  //gl_Position = vec4(inPos, 0.0, 1.0);
  gl_Position = mvp * vec4(inPos, 0.0, 1.0);

  // Pass the interpolated color to the fragment shader
  outUV = inUV;
}

