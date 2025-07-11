#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VERTEX_OUT {
    vec2 TextureCoords;
    vec3 WorldPos;
    vec3 Normal;
} vert_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform bool flipHor;

void main() {
    gl_Position = vec4(aPos, 1.0);
    if (flipHor) {
        vert_out.TextureCoords = aTexCoords;
        vert_out.TextureCoords.x = 1.0 - vert_out.TextureCoords.x;
    } else {
        vert_out.TextureCoords = aTexCoords;
    }
    vert_out.WorldPos = vec3(model * vec4(aPos, 1.0));
    vert_out.Normal = mat3(transpose(inverse(model))) * aNormal;
}