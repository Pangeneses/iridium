#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragWorldPos;

// gl_InstanceIndex = firstInstance + instance, so firstInstance picks the object's slot in the instance table
void main() {
    mat4 model = IR77_MODEL(gl_InstanceIndex);

    vec4 world = model * vec4(inPosition, 1.0);

    fragWorldPos = world.xyz;
    fragNormal = mat3(model) * inNormal;
    fragUV = inUV;

    gl_Position = IR77_CAMERA.view_proj * world;
}
