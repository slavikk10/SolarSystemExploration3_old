#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VERTEX_OUT {
    vec3 WorldPos;
} geom_in[];

out vec3 WorldPos;

void main() {
    gl_Position = gl_in[0].gl_Position;    
    WorldPos = geom_in[0].WorldPos;
    EmitVertex();
    gl_Position = gl_in[1].gl_Position; 
    WorldPos = geom_in[1].WorldPos;
    EmitVertex();
    gl_Position = gl_in[2].gl_Position; 
    WorldPos = geom_in[2].WorldPos;
    EmitVertex();
    EndPrimitive();
}