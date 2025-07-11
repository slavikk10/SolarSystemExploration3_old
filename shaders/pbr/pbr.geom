#version 410 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VERTEX_OUT {
    vec2 TextureCoords;
    vec3 WorldPos;
    vec3 Normal;
} geom_in[];

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

void main() {
    for(int i = 0; i < 3; ++i) {
        gl_Position = gl_in[i].gl_Position;    
        TexCoords = geom_in[i].TextureCoords;
        WorldPos = geom_in[i].WorldPos;
        Normal = geom_in[i].Normal;
        EmitVertex();
    }
    EndPrimitive();
}