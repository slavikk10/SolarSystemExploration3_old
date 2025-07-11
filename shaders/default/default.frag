#version 410 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;
//out vec4 FragColor;

struct Material {


    float shininess;
};

struct PointLight {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct DirectionalLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

in GS_OUT {
    vec2 TexCoords;
    vec3 Normal;
    vec3 FragPos;
    mat3 TBN;
    vec3 lightPos[6];
} fs_in;

uniform vec3 viewPos;
uniform Material material;

uniform DirectionalLight dirLight;

const int NR_LIGHTS = 1;
uniform PointLight pointLights[NR_LIGHTS];
uniform SpotLight spotLight;

// PBR textures
uniform sampler2D albedo;
uniform sampler2D normal;
uniform sampler2D metallic;
uniform sampler2D roughness;
uniform sampler2D ao;

uniform float height_scale;

uniform float far_plane;

vec3 calcDirLight(DirectionalLight light, vec3 normal, vec3 viewDir, float shadow);
vec3 calcPointLight(PointLight light, vec3 normal, vec3 viewDir);
vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 viewDir);

// PBR functions
float DistGGX(vec3 N, vec3 H, float a);
float Gsub_GGX(vec3 N, vec3 V, float k);
float G_GGX(vec3 N, vec3 V, vec3 L, float k);
vec3 Fresnel(float cosTheta, vec3 F0);

float gamma = 2.2;

void main()
{ 
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    //vec2 texCoords = ParallaxMapping(fs_in.TexCoords, viewDir);
    //if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        //discard;

    vec3 norm = texture(normal, fs_in.TexCoords).rgb;
    norm = norm * 2.0 - 1.0;
    norm = normalize(fs_in.TBN * norm);
    //norm = normalize(fs_in.Normal);

    PointLight lights[NR_LIGHTS];
    for(int i = 0; i < NR_LIGHTS; i++) {
        lights[i].position = fs_in.lightPos[i];
        lights[i].ambient = vec3(0.1);
        lights[i].diffuse = vec3(1.0);
        lights[i].specular = vec3(1.0);
        lights[i].constant = 1.0;
        lights[i].linear = 0.09;
        lights[i].quadratic = 0.032;
    }
    //float shadow = ShadowCalculation(vec4(fs_in.FragPos, 1.0), light);

    //vec3 result = calcDirLight(dirLight, norm, viewDir);
    //vec3 result;
    //for(int i = 0; i < 2; i++)
    //    result += calcPointLight(lights[i], norm, viewDir);

    //vec3 result = calcPointLight(light, norm, viewDir);
    vec3 result = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < NR_LIGHTS; i++)
        result += calcPointLight(lights[i], norm, viewDir);

    //DirectionalLight light = DirectionalLight(vec3(1.0f, -1.0f, 0.5f), vec3(0.1f, 0.1, 0.1f), vec3(1.0f, 1.0f, 1.0f), vec3(1.0f, 1.0f, 1.0f));
    //vec3 result = calcDirLight(light, norm, viewDir, shadow);

    //for(int i = 1; i < 4; i++)
            //result += calcPointLight(pointLights[i], norm, viewDir);

    //result += calcSpotLight(spotLight, norm, viewDir);

    float near = 0.1;
    float far = 1000;

    float ndc = gl_FragCoord.z * 2.0 - 1.0;
    float linearDepth = (2.0 * near * far) / (far +  near - ndc * (far - near));


    /*vec3 fragToLight = vec3(fs_in.FragPos) - light.position;
    float closestDepth = texture(shadowMap, fragToLight).r;*/


    //FragColor = vec4(pow(result, vec3(1.0/gamma)), 1.0);
    FragColor = vec4(result, 1.0);
    //FragColor = vec4(norm, 1.0);
    //FragColor = vec4(vec3(closestDepth / far_plane), 1.0);

    //float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    //if(brightness > 1.0)
        //BrightColor = vec4(FragColor.rgb, 1.0);
    //else
        //BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}

vec3 calcDirLight(DirectionalLight light, vec3 normal, vec3 viewDir, float shadow)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);

    // combine results
    vec3 ambient = light.ambient * vec3(texture(albedo, fs_in.TexCoords));

    vec3 diffuse = light.diffuse * diff * vec3(texture(albedo, fs_in.TexCoords));

    vec3 specular = light.specular * spec * vec3(texture(metallic, fs_in.TexCoords));

    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fs_in.FragPos);
    //vec3 lightDir = normalize(light.position - fs_in.FragPos);
    //lightDir = normalize(light.position - fs_in.FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    // diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    //float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32);
    //spec = 1.0;

    // combine results
    vec3 ambient = light.ambient * vec3(texture(albedo, fs_in.TexCoords));

    vec3 diffuse = light.diffuse * diff * vec3(texture(albedo, fs_in.TexCoords));

    vec3 specular = light.specular * spec * vec3(1.0);

    // attenuation
    float distance    = length(light.position - fs_in.FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    //float attenuation = 1.0 / distance;

    //ambient  *= attenuation;
    diffuse   *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}

vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 viewDir)
{
    vec3 result;
    vec3 lightDir = normalize(light.position - fs_in.FragPos);

    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = (light.cutOff - light.outerCutOff);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    // ambient
    vec3 ambient = light.ambient * texture(albedo, fs_in.TexCoords).rgb;

    // diffuse
    vec3 norm = normalize(fs_in.Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(albedo, fs_in.TexCoords).rgb;

    // specular
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * texture(metallic, fs_in.TexCoords).rgb;

    // attenuation
    float distance    = length(light.position - fs_in.FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    diffuse *= intensity;
    specular *= intensity;

    ambient  *= attenuation;
    diffuse   *= attenuation;
    specular *= attenuation;

    result = ambient + diffuse + specular;

    return result;
}

/*float ShadowCalculation(vec4 fragPos, PointLight light) {
    vec3 fragToLight = vec3(fragPos) - light.position;

    float currentDepth = length(fragToLight);

    //float closestToCurrent = currentDepth - closestDepth;

    vec3 sampleOffsetDirections[20] = vec3[]
    (
        vec3( 1, 1, 1), vec3( 1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
        vec3( 1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
        vec3( 1, 1, 0), vec3( 1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
        vec3( 1, 0, 1), vec3(-1, 0, 1), vec3( 1, 0, -1), vec3(-1, 0, -1),
        vec3( 0, 1, 1), vec3( 0, -1, 1), vec3( 0, -1, -1), vec3( 0, 1, -1)
    );

    float shadow = 0.0;
    float bias = 0.15;
    int samples = 20;
    float viewDistance = length(viewPos - vec3(fragPos));
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0;
    diskRadius = 0.15;
    for(int i = 0; i < samples; ++i) {
        float closestDepth = texture(shadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= far_plane;
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);

    return shadow;
}*/

/*vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir) {
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));

    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = viewDir.xy * height_scale;
    vec2 deltaTexCoords = P / numLayers;

    float height = texture(texture_disp, texCoords).r;

    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(texture_disp, currentTexCoords).r;

    while(currentLayerDepth < currentDepthMapValue) {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture(texture_disp, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(texture_disp, prevTexCoords).r - currentLayerDepth + layerDepth;

    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}*/

float DistGGX(vec3 N, vec3 H, float a) {
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14 * denom * denom;

    return nom / denom;
}

float Gsub_GGX(vec3 N, vec3 V, float k) {
    float NdotV = dot(N, V);

    float nom = NdotV;
    float denom = NdotV*(1.0 - k) + k;

    return nom / denom;
}

float G_GGX(vec3 N, vec3 V, vec3 L, float k) {
    float Gsub1 = Gsub_GGX(N, V, k);
    float Gsub2 = Gsub_GGX(N, L, k);

    return Gsub1*Gsub2;
}

vec3 Fresnel(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0)*pow(1.0 - cosTheta, 5.0);
}
