#version 410 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out VERTEX_OUT {
    vec2 TexCoords;
} vert_out;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vert_out.TexCoords = aTexCoords;
}