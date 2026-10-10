#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragWorldPos;

// set 0 binding 0 -- matches Ir77PVCamera
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    mat4 view_proj;
    vec4 eye;
} camera;

// set 0 binding 2 -- one model matrix per instance, written every frame.
// gl_InstanceIndex = firstInstance + instance, so firstInstance picks the object.
layout(std430, set = 0, binding = 2) readonly buffer Instances {
    mat4 models[];
};

void main() {
    mat4 model = models[gl_InstanceIndex];

    vec4 world = model * vec4(inPosition, 1.0);
    fragWorldPos = world.xyz;
    gl_Position = camera.view_proj * world;

    gl_Position = camera.view_proj * model * vec4(inPosition, 1.0);
    fragNormal = mat3(model) * inNormal;
    fragUV = inUV;
}
