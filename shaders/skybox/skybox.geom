#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VERTEX_OUT {
    vec3 localPos;
} geom_in[];

out vec3 localPos;

void main() {
    gl_Position = gl_in[0].gl_Position;    
    localPos = geom_in[0].localPos;
    EmitVertex();
    gl_Position = gl_in[1].gl_Position; 
    localPos = geom_in[1].localPos;
    EmitVertex();
    gl_Position = gl_in[2].gl_Position; 
    localPos = geom_in[2].localPos;
    EmitVertex();
    EndPrimitive();
}