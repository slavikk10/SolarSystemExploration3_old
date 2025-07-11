#version 410 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;
in mat3 TBN;

uniform vec3 camPos;

uniform sampler2D albedoMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform sampler2D normalMap;

uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

uniform samplerCube irradianceMap;

uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

// texture settings
uniform bool invertMetallic;
uniform bool invertRoughness;

uniform bool skipRM;
//uniform bool containsNormalMap;

const float PI = 3.14159265359;

float DistGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float Gsub_GGX(vec3 N, vec3 V, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float NdotV = max(dot(N, V), 0.0);

    float nom = NdotV;
    float denom = NdotV*(1.0 - k) + k;

    return nom / denom;
}

float G_GGX(vec3 N, vec3 V, vec3 L, float roughness) {
    float Gsub1 = Gsub_GGX(N, V, roughness);
    float Gsub2 = Gsub_GGX(N, L, roughness);

    return Gsub1*Gsub2;
}

vec3 Fresnel(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0)*pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    // logarithmic depth buffer
    gl_FragDepth = log2(length(WorldPos) + 1.0) / log2(100000000000000001.0);

    vec3 albedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
    float metallic = 1.0 - texture(metallicMap, TexCoords).r;
    float roughness = 1.0 - max(texture(roughnessMap, TexCoords).r, 0.001);
    if (skipRM) {
        metallic = 0.0;
        roughness = 1.0;
    }
    float ao = texture(aoMap, TexCoords).r;
    ao = 1.0;

    vec3 N = normalize(TBN * (texture(normalMap, TexCoords).rgb * 2.0 - 1.0));
    vec3 V = normalize(camPos - WorldPos);
    vec3 R = reflect(-V, N);

    vec3 irradiance = texture(irradianceMap, N).rgb;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4; ++i) {
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L);
        
        float distance = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;

        vec3 F = Fresnel(max(dot(H, V), 0.0), F0);
        float NDF = DistGGX(N, H, roughness);
        float G = G_GGX(N, V, L, roughness);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 F = FresnelRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 diffuse = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;

    vec2 envBRDF = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    // output final color (convert to LDR + gamma correct)
    FragColor = vec4(pow((ambient + Lo) / ((ambient + Lo) + vec3(1.0)), vec3(1.0/2.2)), 1.0);
}