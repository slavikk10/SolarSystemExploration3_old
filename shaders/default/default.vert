#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec2 TexCoords;
    vec3 Normal;
    vec3 FragPos;
    mat3 TBN;
    vec3 lightPos[6];
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat3 normalTransposed;

uniform vec3 lightPos[6];
uniform vec3 viewPos;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.Normal = mat3(transpose(inverse(model))) * aNormal;
    vs_out.TexCoords = aTexCoords;

    vec3 T = normalize(vec3(model * vec4(aTangent,   0.0)));
    vec3 N = normalize(vec3(model * vec4(aNormal,    0.0)));

    T = normalize(T - dot(T, N) * N);

    vec3 B = cross(N, T);

    mat3 TBN = mat3(T, B, N);
    
    vs_out.TBN = TBN;

    for (int i = 0; i < 6; i++)
        vs_out.lightPos[i] = lightPos[i];

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}