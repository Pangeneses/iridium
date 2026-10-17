#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"

layout(location = 0) in vec2 fragUV;  // unused -- kept so the interface still matches cef.vert
layout(location = 0) out vec4 outColor;

// pc.user[0] = the overlay's texture slot
#define CEF_TEXTURE IR77_TEXTURE(pc.user[0])

vec3 applyContrast(vec3 color) {
    return ((color - 0.5) * 1.0) + 0.5;
}

vec3 applyGamma(vec3 color) {
    return pow(color, vec3(1.0 / 0.45));
}

void main() {
    // 1:1 pixels -- the last painted CEF frame is never stretched to the window. While CEF catches up after a resize,
    // a too-small frame leaves the scene showing at the edge and a too-large one is cropped; the UI never scales.
    ivec2 size = textureSize(CEF_TEXTURE, 0);
    ivec2 pixel = ivec2(gl_FragCoord.xy);

    if (pixel.x >= size.x || pixel.y >= size.y) discard;

    vec4 tex = texelFetch(CEF_TEXTURE, pixel, 0);

    vec3 c = tex.rgb;
    c = applyContrast(c);
    c = applyGamma(c);

    outColor = vec4(c, tex.a);
}
