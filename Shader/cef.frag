#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D cefTexture;

vec3 applyContrast(vec3 color) {
    return ((color - 0.5) * 1.0) + 0.5;
}

vec3 applyGamma(vec3 color) {
    return pow(color, vec3(1.0 / 0.45));
}

void main() {
    vec4 tex = texture(cefTexture, fragUV);

    vec3 c = tex.rgb;
    c = applyContrast(c);
    c = applyGamma(c);

    outColor = vec4(c, tex.a);
}


