#version 450

// Input from the vertex shader
layout(location = 0) in vec4 fragColor;

// Output to the framebuffer
layout(location = 0) out vec4 color;

// Uniform for alpha value
//layout(binding = 1) uniform Alpha {
    //float alpha;
//};

void main() {
    // Set the fragment color with alpha
    //color = vec4(fragColor, alpha);
    color = fragColor;
}

