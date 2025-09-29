#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec4 tangent; // xyz + w(sign)

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 fragUV;
layout(location = 4) out vec4 fragTangentWorld; // pass w too
layout(location = 5) out float vViewZ;

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

layout(std430, set = 2, binding = 0) buffer LightBuffer {
    uint lightCount; uint _pad0; uint _pad1; uint _pad2;
    Light lights[];
} gLights;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    int textureIndex;
    int normalIndex;
    int pad1;
    int pad2;
} push;

const float AMBIENT = 0.09;

const mat4 biasMat = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

void main() {
    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
    fragPosWorld = positionWorld.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(push.modelMatrix)));

    vec3 N = normalize(normalMatrix * normal);

    fragNormalWorld = N;
    fragColor = color;
    fragUV = uv;

    vec3 T = normalize(normalMatrix * tangent.xyz);

    // Orthonormalize T against N to kill imported drift
    T = normalize(T - N * dot(N,T));

    fragTangentWorld = vec4(T, tangent.w);

    // Position of vertex as seen from directional light
    vec4 viewPos = ubo.viewMatrix * positionWorld;
    vViewZ = viewPos.z;

    gl_Position = ubo.projectionMatrix * viewPos;
}