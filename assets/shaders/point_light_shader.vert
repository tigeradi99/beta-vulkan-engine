#version 450

const vec2 OFFSETS[6] = vec2[](
  vec2(1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(-1.0, -1.0),
  vec2(1.0, 1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0)
);

layout (location = 0) out vec2 fragOffset;
layout (location = 1) flat out uint vLightIndex;  // which light this instance is

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

const float LIGHT_RADIUS = 0.1;

void main() {
    fragOffset = OFFSETS[gl_VertexIndex];
    vLightIndex = gl_InstanceIndex + 1;

    // Fetch this instance's light:
    Light L = gLights.lights[vLightIndex];

    // Only draw for point lights
    // type: 1 = point
    vec3 posWorld = L.position.xyz;
    vec4 lightPosCamera  = ubo.viewMatrix * vec4(posWorld, 1.0);

    vec4 positionViewBillboard = lightPosCamera + LIGHT_RADIUS * vec4(fragOffset, 0.0, 0.0);

    gl_Position = ubo.projectionMatrix * positionViewBillboard;
}