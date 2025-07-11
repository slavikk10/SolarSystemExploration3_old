#version 410 core
out vec4 FragColor;
in vec3 WorldPos;
in vec4 vClipPos;

const float densityFalloff = 10.0;
const float scatteringStrength = 0.01;

uniform vec3 camPos;
uniform vec3 lightPos;

uniform vec3 planetWorldPos;
uniform float planetRadius;

uniform float atmosphereHeight;

uniform vec3 wavelengths;

float atmosphereRadius = planetRadius + atmosphereHeight;

float scatterR = pow(400 / wavelengths.x, 4) * scatteringStrength;
float scatterG = pow(400 / wavelengths.y, 4) * scatteringStrength;
float scatterB = pow(400 / wavelengths.z, 4) * scatteringStrength;
vec3 scatteringCoefficients = vec3(scatterR, scatterG, scatterB);

const int NUM_SAMPLES = 10;

uniform sampler2D depthTex;
uniform sampler2D colorTex;

// returns (dstToAtmosphere, dstThroughAtmosphere)
vec2 raySphere(vec3 planetWorldPos, float atmosphereRadius, vec3 rayOrigin, vec3 rayDir) {
    vec3 offset = rayOrigin - planetWorldPos;
    float a = 1;
    float b = 2 * dot(offset, rayDir);
    float c = dot(offset, offset) - atmosphereRadius * atmosphereRadius;
    float d = b * b - 4 * a * c;

    if (d > 0) {
        float s = sqrt(d);
        float dstToSphereNear = max(0, (-b - s) / (2 * a));
        float dstToSphereFar = (-b + s) / (2 * a);

        if (dstToSphereFar >= 0) {
            return vec2(dstToSphereNear, dstToSphereFar - dstToSphereNear);
        }
    }

    // ray did not hit the atmosphere
    return vec2(100000000000000000.0, 0.0);
}

float density(vec3 samplePoint) {
    float height = length(samplePoint - planetWorldPos) - planetRadius;
    return exp(-(height / (atmosphereRadius - planetRadius)) * densityFalloff) * (1 - (height / (atmosphereRadius - planetRadius)));
}

float opticalDepth(vec3 rayOrigin, vec3 rayDir, float rayLength) {
    vec3 densitySamplePoint = rayOrigin;
    float stepSize = rayLength / (NUM_SAMPLES - 1);
    float opticalDepth = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        float density = density(densitySamplePoint);
        opticalDepth += density;
        densitySamplePoint += rayDir * stepSize;
    }
    return opticalDepth;
}

vec3 calculateLight(vec3 rayOrigin, vec3 rayDir, float rayLength, vec3 originalColor) {
    vec3 inScatterPoint = rayOrigin;
    float stepSize = rayLength / (NUM_SAMPLES - 1);
    vec3 inScatteredLight = vec3(0.0);
    float viewRayOpticalDepth = 0.0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        float sunRayLength = raySphere(planetWorldPos, atmosphereRadius, inScatterPoint, normalize(lightPos - WorldPos)).y;
        float sunRayOpticalDepth = opticalDepth(inScatterPoint, normalize(lightPos - WorldPos), sunRayLength);
        viewRayOpticalDepth = opticalDepth(inScatterPoint, -rayDir, stepSize * i);
        vec3 transmittance = exp(-(sunRayOpticalDepth + viewRayOpticalDepth) * scatteringCoefficients);
        float density = density(inScatterPoint);

        inScatteredLight += density * transmittance * scatteringCoefficients * stepSize;
        inScatterPoint += rayDir * stepSize;
    }

    float originalColorTransmittance = exp(-viewRayOpticalDepth);
    return originalColor * originalColorTransmittance + inScatteredLight;
}

void main() {
    gl_FragDepth = log2(length(WorldPos) + 1.0) / log2(1000000000000001.0);
    
    vec2 TexCoords = (vClipPos.xyz / vClipPos.w).xy * 0.5 + 0.5;
    vec4 originalCol = texture(colorTex, TexCoords);

    float linearDepth = (pow(2.0, texture(depthTex, TexCoords).r * 56.472) - 1.0);
    linearDepth = linearDepth * length(WorldPos);
    
    vec3 viewDir = normalize(WorldPos);
    vec2 hitInfo = raySphere(planetWorldPos, atmosphereRadius, vec3(0.0), viewDir);

    float dstToAtmosphere = hitInfo.x;
    float dstThroughAtmosphere = min(hitInfo.y, linearDepth - dstToAtmosphere);

    if (dstThroughAtmosphere > 0) {
        vec3 pointInAtmosphere = vec3(0.0) + viewDir * hitInfo.x;
        vec3 light = calculateLight(pointInAtmosphere, viewDir, min(hitInfo.y, linearDepth - hitInfo.x), texture(colorTex, TexCoords).rgb);
        FragColor = vec4(light, 0);
    } else {
        FragColor = originalCol;
    }
}