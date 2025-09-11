#version 410 core
out vec4 FragColor;

void main() 
{
    gl_FragDepth = 0.0;
    FragColor = vec4(1.0);
}