#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out VERTEX_OUT {
    vec3 localPos;
} vert_out;

void main() {
    vert_out.localPos = aPos;

    mat4 rotView = mat4(mat3(view)); // remove translation
    vec4 clipPos = projection * rotView * vec4(aPos, 1.0);

    gl_Position = clipPos.xyww;
}