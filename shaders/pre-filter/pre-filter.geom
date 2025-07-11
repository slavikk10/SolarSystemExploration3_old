#version 410 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VERTEX_OUT {
    vec3 localPos;
} geom_in[];

out vec3 localPos;

void main() {
    for(int i = 0; i < 3; ++i) {
        gl_Position = gl_in[i].gl_Position;    
        localPos = geom_in[i].localPos;
        EmitVertex();
    }
    EndPrimitive();
}