#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    // Textures are sampled using the built-in texture function.
    // It takes a sampler and coordinate as arguments. 
    // The sampler automatically takes care of the filtering and transformations in the background
    // outColor = vec4(1,1,1,1);
    outColor = texture(texSampler, fragTexCoord);
}