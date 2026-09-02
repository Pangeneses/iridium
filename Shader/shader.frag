#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
    // simple placeholder: visualize the normal as color, ignoring UV for now
    outColor = vec4(normalize(fragNormal) * 0.5 + 0.5, 1.0);
}
