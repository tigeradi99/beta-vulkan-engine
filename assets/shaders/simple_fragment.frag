#version 450

layout(location = 0) in vec3 color;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragUV;
layout(location = 4) in vec4 fragTangentWorld;
layout(location = 5) in float vViewZ;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform globalUbo {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    vec4 ambientLightColor;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D texSamplers[1000];

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

const int NUM_CASCADES = 4;

layout(set = 3, binding = 0) uniform lightUbo {
    mat4 lightProjectionMatrix[NUM_CASCADES];
    mat4 lightViewMatrix[NUM_CASCADES];
    float splitDepths[NUM_CASCADES];
} sUbo;

layout(set = 3, binding = 1) uniform sampler2DArrayShadow shadowMap;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    int textureIndex;
    int normalIndex;
    int pad1;
    int pad2;
} push;

vec3 lambert(vec3 n, vec3 l, vec3 lightRgb, float intensity, float ndotl, vec3 albedo) {
    return lightRgb * intensity * ndotl * albedo;
}

vec3 blinnPhong(vec3 n, vec3 l, vec3 v, vec3 lightRgb, float intensity, float specPow) {
    vec3 h = normalize(l + v);
    float nh = max(dot(n, h), 0.0);
    float spec = pow(nh, specPow);
    return lightRgb * intensity * spec;
}

void evalLight(in Light L, in vec3 P, in vec3 N, in vec3 V, in vec3 albedo, out vec3 outDiff, out vec3 outSpec) {
    outDiff = vec3(0.0); outSpec = vec3(0.0);

    int   type      = int(L.position.w + 0.5);
    vec3  lightRgb  = L.color.rgb;
    float intensity = L.color.a;
    float specPow   = (L.params.w != 0.0) ? L.params.w : 32.0;

    vec3  Ldir;
    float atten = 1.0;

    if (type == 0) {
        // Directional: no attenuation
        Ldir = normalize(L.direction.xyz);
    } else if (type == 1) {
        // Point: inverse-square * smooth range cutoff
        vec3 toLight = L.position.xyz - P;
        float d2 = dot(toLight, toLight);
        float d  = sqrt(d2);
        Ldir = toLight / max(d, 1e-6);
        float range = max(L.params.x, 1e-4);
        float rf = clamp(1.0 - d / range, 0.0, 1.0);
        atten = (1.0 / max(d2, 1e-6)) * rf * rf;
    } else { // Spot
        vec3 toLight = L.position.xyz - P;
        float d2 = dot(toLight, toLight);
        float d  = sqrt(d2);
        Ldir = toLight / max(d, 1e-6);

        float cosTheta = dot(normalize(-L.direction.xyz), Ldir);
        float innerC = L.params.y;
        float outerC = L.params.z;
        float spot = clamp((cosTheta - outerC) / max(innerC - outerC, 1e-6), 0.0, 1.0);

        float range = max(L.params.x, 1e-4);
        float rf = clamp(1.0 - d / range, 0.0, 1.0);
        atten = spot * (1.0 / max(d2, 1e-6)) * rf * rf;
    }

    float ndotl = max(dot(N, Ldir), 0.0);
    if (ndotl > 0.0 && intensity > 0.0 && atten > 0.0) {
        outDiff += lambert(N, Ldir, lightRgb, intensity * atten, ndotl, albedo);
        outSpec += blinnPhong(N, Ldir, V, lightRgb, intensity * atten, specPow);
    }
}

int pickCascade(float zView) {
    for (int i = 0; i < NUM_CASCADES; ++i) {
        if (zView <= sUbo.splitDepths[i]) return i;
    }
    return NUM_CASCADES - 1;
}

float sampleShadowCSM(vec3 worldPos, int cascade) {
    mat4 viewProj = sUbo.lightProjectionMatrix[cascade] * sUbo.lightViewMatrix[cascade];
    vec4 posLS = viewProj * vec4(worldPos, 1.0);
    posLS.xyz /= posLS.w;

    vec2 uv = posLS.xy * 0.5 + 0.5;

    // quick reject outside map
    if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)))) {
        return 1.0;
    }

    float ref = posLS.z;
    return texture(shadowMap, vec4(uv, float(cascade), ref));
}

// Optional: flip green if your normal maps are in "DirectX" convention
const bool FLIP_GREEN = false;

vec3 sampleWorldNormal() {
    // Sample tangent-space normal from texture (UNORM)
    vec3 n_ts = texture(texSamplers[push.normalIndex], fragUV).xyz * 2.0 - 1.0;
    if (FLIP_GREEN) n_ts.g = -n_ts.g;

    // Build TBN (bitangent from cross * handedness)
    vec3 N = normalize(fragNormalWorld);
    vec3 T = normalize(fragTangentWorld.xyz);
    vec3 B = normalize(cross(N, T)) * fragTangentWorld.w;

    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * n_ts);
}

void main() {
    int cascade_index = pickCascade(vViewZ);
    float shadow = sampleShadowCSM(fragPosWorld, cascade_index);
    shadow = mix(0.3, 1.0, shadow);

    vec3 texColor = texture(texSamplers[push.textureIndex], fragUV).rgb;
    vec3 albedo = texColor * color;

    vec3 N = sampleWorldNormal(); // fetch normal from normal map
    vec3 cameraPosWorld = ubo.inverseViewMatrix[3].xyz;
    vec3 V = normalize(cameraPosWorld - fragPosWorld); // View Direction

    // Ambient
    vec3 diffuseLight = ubo.ambientLightColor.rgb * ubo.ambientLightColor.a * albedo;
    vec3 specularLight = vec3(0.0);

   // Loop over lights from SSBO
   uint numLights = gLights.lightCount;
   // (Optional) clamp to a sane upper bound to avoid pathological CPU bugs.
   numLights = min(numLights, 1024u);

   for (uint i = 0u; i < numLights; ++i) {
       vec3 dC, sC;
       evalLight(gLights.lights[i], fragPosWorld, N, V, albedo, dC, sC);
       diffuseLight += dC;
       specularLight += sC;
   }

   outColor = shadow * vec4(diffuseLight + specularLight, 1.0);
}