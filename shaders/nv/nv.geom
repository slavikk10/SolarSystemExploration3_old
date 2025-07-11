#version 410 core
layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in VERTEX_OUT {
    vec3 normal;
} geom_in[];

const float MAGNITUDE = 1.0;

uniform mat4 projection;

void genLine(int index) {
    gl_Position = projection * gl_in[index].gl_Position;
    EmitVertex();
    gl_Position = projection * (gl_in[index].gl_Position + vec4(geom_in[index].normal, 0.0) * MAGNITUDE);
    EmitVertex();
    EndPrimitive();
}

void main() {
    genLine(0);
    genLine(1);
    genLine(2);
}