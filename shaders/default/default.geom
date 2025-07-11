#version 410 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec2 TexCoords;
    vec3 Normal;
    vec3 FragPos;
    mat3 TBN;
    vec3 lightPos[6];
} gs_in[];

out GS_OUT {
    vec2 TexCoords;
    vec3 Normal;
    vec3 FragPos;
    mat3 TBN;
    vec3 lightPos[6];
} gs_out;

void main() {
    for(int i = 0; i < 3; ++i) {
        gl_Position = gl_in[i].gl_Position;    
        gs_out.TexCoords = gs_in[i].TexCoords;
        gs_out.Normal = gs_in[i].Normal;
        gs_out.FragPos = gs_in[i].FragPos;
        gs_out.TBN = gs_in[i].TBN;
        for(int i = 0; i < 6; i++)
            gs_out.lightPos[i] = gs_in[i].lightPos[i];
        EmitVertex();
    }
    EndPrimitive();
}