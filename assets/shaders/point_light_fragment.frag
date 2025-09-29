#version 450

layout (location = 0) in vec2 fragOffset;
layout(location = 1) flat in uint vLightIndex;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform globalUbo {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    vec4 ambientLightColor;
} ubo;

struct Light {
    vec4 color;      // rgb, a = intensity
    vec4 position;   // xyz, w = type (0 dir, 1 point, 2 spot)
    vec4 direction;  // xyz
    vec4 params;     // x=range, y=innerConeCos, z=outerConeCos, w=specPower
};

layout(std430, set = 1, binding = 0) buffer LightBuffer {
    uint lightCount; uint _pad0; uint _pad1; uint _pad2;
    Light lights[];
} gLights;

void main() {
    float dist = sqrt(dot(fragOffset, fragOffset));
    if (dist >= 1.0) {
        discard;
    }

    Light L = gLights.lights[vLightIndex];
    vec3 rgb = L.color.rgb * L.color.a;

    outColor = vec4(rgb, 1.0);
}