#version 460
#extension GL_GOOGLE_include_directive : require

// The field's main-pass vertex shader -- surface or particles from the point buffer the field program wrote (field_vertex.glsl)
#define IR77_STAGE_VERTEX
#include "include/bindless.glsl"
#include "include/field.glsl"
#include "include/field_vertex.glsl"

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec2 fragCoord;
layout(location = 4) out float fragValue;
layout(location = 5) out vec2 fragCorner;
layout(location = 6) out vec4 fragInk;

void main() {
    Ir77FieldVertex v = Ir77BuildFieldVertex(uint(gl_VertexIndex));

    fragNormal = v.normal;
    fragColor = v.color;
    fragWorldPos = v.world;
    fragCoord = v.coord;
    fragValue = v.value;
    fragCorner = v.corner;
    fragInk = v.ink;

    gl_Position = IR77_CAMERA.view_proj * vec4(v.world, 1.0);
}
