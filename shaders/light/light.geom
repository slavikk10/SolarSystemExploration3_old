#version 410 core
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

in VERTEX_OUT {
    vec3 WorldPos;
} vert_out[];

out vec3 WorldPos;

void main() {
    for(int i = 0; i < 3; ++i) {
        gl_Position = gl_in[i].gl_Position;
        WorldPos = vert_out[i].WorldPos;
        EmitVertex();
    }
    EndPrimitive();
}