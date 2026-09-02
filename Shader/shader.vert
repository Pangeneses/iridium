#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUV;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 projection;
    mat4 view;
    mat4 model;
} camera;

void main() {
    gl_Position = camera.projection * camera.view * camera.model * vec4(inPosition, 1.0);
    fragNormal = mat3(camera.model) * inNormal;
    fragUV = inUV;
}
