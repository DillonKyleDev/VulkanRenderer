#version 450


layout(location = 0) out vec3 fragColor;

vec3 positions[8] = vec3[] (
    vec3(-0.5, -0.5, 0.0), // top left front
    vec3(-0.5, 0.5, 0.0), // top right front
    vec3(0.5, -0.5, 0.0), // bottom left front
    vec3(0.5, 0.5, 0.0), // bottom right front
    vec3(-0.5, -0.5, 0.5), // top left front
    vec3(-0.5, 0.5, 0.5), // top right front
    vec3(0.5, -0.5, 0.5), // bottom left front
    vec3(0.5, 0.5, 0.5) // bottom right front
);
vec3 colors[3] = vec3[] (
    vec3(1.0, 1.0, 1.0),
    vec3(1.0, 1.0, 1.0),
    vec3(1.0, 1.0, 1.0)
);

void main() 
{
    // our output variable
    gl_Position = vec4(positions[gl_VertexIndex], 1.0);
    fragColor = colors[gl_VertexIndex];
}