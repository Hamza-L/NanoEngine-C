#version 450

//INPUTS--------------------------
layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexUV;

//OUTPUTS-------------------------
layout(location = 0) out vec4 outColor;

//GLOBALS
layout(binding = 1) uniform sampler2D texSampler;

void main() {
    // outColor = texture(texSampler, fragTexUV);
    outColor = fragColor;
}
