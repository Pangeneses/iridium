#version 460
#extension GL_GOOGLE_include_directive : require

// The field surface into a shadow atlas tile -- pc.user[0] = which shadow matrix. Depth only.
#define IR77_STAGE_VERTEX
#include "include/bindless.glsl"
#include "include/field.glsl"
#include "include/field_vertex.glsl"

void main() {
    Ir77FieldVertex v = Ir77BuildFieldVertex(uint(gl_VertexIndex));

    gl_Position = IR77_SHADOW_MATRIX(pc.user[0]) * vec4(v.world, 1.0);
}
