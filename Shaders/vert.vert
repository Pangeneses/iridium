#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 5) in vec4 inTangent;  // xyz along +U, w = handedness

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) flat out uint fragMaterial;
layout(location = 4) out vec4 fragTangent;  // xyz world, w handedness -- flipped when the model mirrors

// per-draw material table: pc.user[4] = table slot, pc.user[5] = first command of this draw
IR77_BUFFER(ir77_draw_materials, uint);

// gl_InstanceIndex = firstInstance + instance, so firstInstance picks the object's slot in the instance table
void main() {
    mat4 model = IR77_MODEL(gl_InstanceIndex);
    mat3 normal_matrix = mat3(model);

    vec4 world = model * vec4(inPosition, 1.0);

    fragWorldPos = world.xyz;
    fragNormal = normal_matrix * inNormal;
    fragUV = inUV;
    fragTangent = vec4(normal_matrix * inTangent.xyz, determinant(normal_matrix) < 0.0 ? -inTangent.w : inTangent.w);

    fragMaterial = ir77_draw_materials[nonuniformEXT(pc.user[4])].data[pc.user[5] + gl_DrawID];

    gl_Position = IR77_CAMERA.view_proj * world;
}
