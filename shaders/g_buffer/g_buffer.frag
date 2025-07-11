#version 410 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

uniform bool inverseNormals;

void main() {
    gPosition = FragPos;
    gNormal = inverseNormals == false ? normalize(Normal) : normalize(-Normal);
    gAlbedoSpec.rgb = vec3(0.95);
}