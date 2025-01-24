#version 450

// Vertex attributes
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;

// Output to fragment shader
layout(location = 0) out vec4 outColor;

// Uniforms for transformation matrices
//layout(binding = 0) uniform Matrices {
  //mat4 model;
  //mat4 view;
  //mat4 projection;
//};

void main() {
  // Calculate the final position of the vertex
  // gl_Position = projection * view * model * vec4(aPos, 1.0);
  gl_Position = vec4(inPos, 1.0);

  // Pass the interpolated color to the fragment shader
  outColor = inColor;
}

