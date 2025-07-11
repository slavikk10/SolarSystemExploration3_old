#version 410 core
layout (triangles, equal_spacing, ccw) in;

uniform sampler2D heightMap;  // the texture corresponding to our height map
uniform mat4 model;           // the model matrix
uniform mat4 view;            // the view matrix
uniform mat4 projection;      // the projection matrix
uniform mat3 rotationMatrix;  // the rotation matrix (to rotate the TBN matrix)

// received from Tessellation Control Shader - all texture coordinates for the patch vertices
in vec2 TextureCoorde[];
in vec3 WorldPose[];
in vec3 Normale[];

// send to Fragment Shader
out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;
out mat3 TBN;

void main() {
    // get patch coordinate
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    // ----------------------------------------------------------------------
    // retrieve control point texture coordinates
    vec2 t00 = TextureCoorde[0];
    vec2 t01 = TextureCoorde[1];
    vec2 t10 = TextureCoorde[2];

    // interpolate texture coordinate across patch
    vec2 texCoord = t00 * (1.0 - u - v) + t01 * u + t10 * v;

    // ----------------------------------------------------------------------
    // retrieve control point world positions
    vec3 w00 = WorldPose[0];
    vec3 w01 = WorldPose[1];
    vec3 w10 = WorldPose[2];

    // interpolate world position across patch
    vec3 worldPos = w00 * (1.0 - u - v) + w01 * u + w10 * v;

    // lookup texel at patch coordinate for height and scale + shift as desired
    float Height = texture(heightMap, texCoord).r * 0.0025;

    // ----------------------------------------------------------------------
    // retrieve control point position coordinates
    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;

    // ----------------------------------------------------------------------
    // retrieve control point normals
    vec3 n00 = Normale[0];
    vec3 n01 = Normale[1];
    vec3 n10 = Normale[2];

    // interpolate normal across patch
    vec3 normal = normalize(n00 * (1.0 - u - v) + n01 * u + n10 * v);
    // calculate edges of the triangle
    vec3 e1 = p01.xyz - p00.xyz;
    vec3 e2 = p10.xyz - p00.xyz;
    // calculate delta UV coordinates of the triangle
    vec2 dUV1 = t01 - t00;
    vec2 dUV2 = t10 - t00;
    // calculate tangent and bitangent
    float f = 1.0 / (dUV1.x * dUV2.y - dUV2.x * dUV1.y);
    vec3 tangent = vec3(0.0);
    tangent.x = f * (dUV2.y * e1.x - dUV1.y * e2.x);
    tangent.y = f * (dUV2.y * e1.y - dUV1.y * e2.y);
    tangent.z = f * (dUV2.y * e1.z - dUV1.y * e2.z);
    tangent = normalize(tangent);
    // when we have two perpendicular vectors, we can easily find the third by finding cross product of the two perpendicular vectors that we currently have:
    vec3 bitangent = normalize(cross(normal, tangent));

    // calculate TBN matrix
    vec3 T = vec3(model * vec4(tangent, 0.0)); // note that we do not have to normalize the result because tangent is already normalized
    vec3 B = vec3(model * vec4(bitangent, 0.0));
    vec3 N = vec3(model * vec4(normal, 0.0));
    TBN = rotationMatrix * mat3(T, B, N);

    // interpolate position across patch
    vec4 p = p00 * (1.0 - u - v) + p01 * u + p10 * v;

    // displace point along normal
    p.xyz += normal * Height;

    // send variables to Fragment Shader
    WorldPos = worldPos;
    TexCoords = texCoord;
    Normal = normal;

    // ----------------------------------------------------------------------
    // output patch point position in clip space
    gl_Position = projection * view * model * p;
}