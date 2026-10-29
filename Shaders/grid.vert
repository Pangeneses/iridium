#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"

// The editor grid -- one fullscreen triangle per view, no vertex buffer. Each corner goes out as the points the view's camera sees there at
// the near (1) and far (0) planes -- reversed-Z -- still homogeneous, so the fragment shader divides per pixel and gets the exact ray.
layout(location = 0) noperspective out vec4 v_near;
layout(location = 1) noperspective out vec4 v_far;

void main() {
    vec2 p = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0 - 1.0;  // (-1,-1) (3,-1) (-1,3)

    mat4 inv = inverse(IR77_CAMERA.view_proj);
    v_near = inv * vec4(p, 1.0, 1.0);
    v_far = inv * vec4(p, 0.0, 1.0);

    gl_Position = vec4(p, 0.0, 1.0);
}
