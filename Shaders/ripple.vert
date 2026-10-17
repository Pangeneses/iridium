#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragWorldPos;

// concentric ripple: h(r, t) = A cos(k r - k speed t), r = distance from the object's origin in its X/Z plane (glTF is Y-up)
const float amplitude  = 0.1;  // height of the waves
const float wavelength = 0.5;  // distance between crests
const float speed      = 1.0;  // how fast the crests move outward, units per second

// gl_InstanceIndex = firstInstance + instance, so firstInstance picks the object's slot in the instance table
void main() {
    mat4 model = IR77_MODEL(gl_InstanceIndex);

    // displace in object space, before the model transform -- the ripple follows the object
    vec3 position = inPosition;

    float r     = length(position.xz);
    float k     = 6.2831853 / wavelength;
    float phase = k * (r - speed * IR77_CAMERA.time.x);

    position.y += amplitude * cos(phase);

    // normal from the height gradient: dh/dx = -A k sin(phase) x / r, same for z (0 at the centre)
    vec2 slope  = r > 1e-5 ? (-amplitude * k * sin(phase) / r) * inPosition.xz : vec2(0.0);
    vec3 normal = normalize(vec3(-slope.x, 1.0, -slope.y));

    vec4 world = model * vec4(position, 1.0);

    fragWorldPos = world.xyz;
    fragNormal = mat3(model) * normal;
    fragUV = inUV;

    gl_Position = IR77_CAMERA.view_proj * world;
}
