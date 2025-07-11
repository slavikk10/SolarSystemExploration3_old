#version 410 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VERTEX_OUT {
    vec2 TexCoords;
} geom_in[];

out GEOMETRY_OUT {
    vec2 TexCoords;
} geom_out;

void main() {
    for(int i = 0; i < 3; ++i) {
        gl_Position = gl_in[i].gl_Position;
        geom_out.TexCoords = geom_in[i].TexCoords;
        EmitVertex();
    }
    EndPrimitive();
}