#version 450

// Input from the vertex shader
layout(location = 0) in vec2 UV;

// Output to the framebuffer
layout(location = 0) out vec4 color;

// Uniform for alpha value
//layout(binding = 1) uniform Alpha {
    //float alpha;
//};

layout(set = 2, binding = 0) uniform sampler2D texsampler;

void main() {
  // Set the fragment color with alpha
  //color = vec4(fragColor, alpha);
  //color = fragColor;
  //color = vec4(UV, 1.0, 1.0);
  color = texture(texsampler, UV);
}

