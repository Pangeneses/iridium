#version 450

layout(location = 0) in vec2 fragUV;  // unused -- kept so the interface still matches cef.vert
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D cefTexture;

vec3 applyContrast(vec3 color) {
    return ((color - 0.5) * 1.0) + 0.5;
}

vec3 applyGamma(vec3 color) {
    return pow(color, vec3(1.0 / 0.45));
}

void main() {
    // 1:1 pixels -- the last painted CEF frame is never stretched to the window. While CEF catches up after a resize,
    // a too-small frame leaves the scene showing at the edge and a too-large one is cropped; the UI never scales.
    ivec2 size = textureSize(cefTexture, 0);
    ivec2 pixel = ivec2(gl_FragCoord.xy);

    if (pixel.x >= size.x || pixel.y >= size.y) discard;

    vec4 tex = texelFetch(cefTexture, pixel, 0);

    vec3 c = tex.rgb;
    c = applyContrast(c);
    c = applyGamma(c);

    outColor = vec4(c, tex.a);
}