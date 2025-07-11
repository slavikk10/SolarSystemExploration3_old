#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

in VERTEX_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
} geom_in[];

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    for(int i = 0; i < 3; ++i) {
        gl_Position = gl_in[i].gl_Position;
        FragPos = geom_in[i].FragPos;
        TexCoords = geom_in[i].TexCoords;
        Normal = geom_in[i].Normal;
        EmitVertex();
    }
    EndPrimitive();
}