#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VERTEX_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
} vert_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    vert_out.FragPos = worldPos.xyz; 
    vert_out.TexCoords = aTexCoords;
    
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vert_out.Normal = normalMatrix * aNormal;

    gl_Position = projection * view * worldPos;
}