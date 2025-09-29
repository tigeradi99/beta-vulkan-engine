#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec4 tangent; // xyz + w(sign)

const int NUM_CASCADES = 4;

layout(set = 0, binding = 0) uniform lightUbo {
    mat4 lightProjectionMatrix[NUM_CASCADES];
    mat4 lightViewMatrix[NUM_CASCADES];
    float splitDepths[NUM_CASCADES];
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    int cascadeIndex;
} push;

void main() {
    int cascade_index = push.cascadeIndex;
    mat4 viewProj = ubo.lightProjectionMatrix[cascade_index] * ubo.lightViewMatrix[cascade_index];
    gl_Position = viewProj * push.modelMatrix * vec4(position, 1.0);
}