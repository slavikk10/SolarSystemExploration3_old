#version 410 core
out vec4 FragColor;

in GEOMETRY_OUT {
    vec2 TexCoords;
} frag_in;

uniform sampler2D hdrTex;

void main() {
    const float gamma = 2.2;
    vec3 hdrColor = texture(hdrTex, frag_in.TexCoords).rgb;

    vec3 mapped = hdrColor / (hdrColor + vec3(1.0));

    mapped = pow(mapped, vec3(1.0 / gamma));

    FragColor = vec4(mapped, 1.0);
    FragColor = texture(hdrTex, frag_in.TexCoords);
}