#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in uvec4 inJoints;
layout(location = 4) in vec4 inWeights;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragWorldPos;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    mat4 view_proj;
    vec4 eye;
} camera;

layout(std430, set = 0, binding = 2) readonly buffer Instances {
    mat4 models[];
};

// bone_base[instance] = where that instance's joint palette starts in bones[]
layout(std430, set = 0, binding = 4) readonly buffer Bones {
    uint bone_base[8192];
    mat4 bones[];
};

void main() {
    uint base = bone_base[gl_InstanceIndex];

    // up to four joints per vertex; glTF weights sum to 1
    mat4 skin = inWeights.x * bones[base + inJoints.x] +
                inWeights.y * bones[base + inJoints.y] +
                inWeights.z * bones[base + inJoints.z] +
                inWeights.w * bones[base + inJoints.w];

    mat4 model = models[gl_InstanceIndex] * skin;

    vec4 world = model * vec4(inPosition, 1.0);
    fragWorldPos = world.xyz;
    fragNormal = mat3(model) * inNormal;
    fragUV = inUV;

    gl_Position = camera.view_proj * world;
}