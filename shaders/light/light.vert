#version 410 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out VERTEX_OUT {
    vec3 WorldPos;
} vert_out;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vert_out.WorldPos = vec3(model * vec4(aPos, 1.0));
}