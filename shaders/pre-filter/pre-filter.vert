#version 410 core
layout (location = 0) in vec3 aPos;

out VERTEX_OUT {
    vec3 localPos;
} vert_out;

uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * vec4(aPos, 1.0);
    vert_out.localPos = aPos;
}