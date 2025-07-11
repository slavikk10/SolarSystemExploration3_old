#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out VERTEX_OUT {
    vec3 WorldPos;
} vert_out;

void main() {
    vert_out.WorldPos = aPos;
    gl_Position = projection * view * vec4(vert_out.WorldPos, 1.0);
}