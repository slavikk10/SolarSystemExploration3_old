#version 330 core
layout (location = 0) in vec3 aPos;

out VS_OUT {
    vec3 localPos;
} vert_out;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    vert_out.localPos = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}