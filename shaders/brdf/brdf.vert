#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out VERTEX_OUT {
    vec2 TexCoords;
} vert_out;

void main() {
    vert_out.TexCoords = aTexCoords;
	gl_Position = vec4(aPos, 1.0);
}