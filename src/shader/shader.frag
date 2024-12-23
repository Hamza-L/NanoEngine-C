#version 450

//INPUTS--------------------------
layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexUV;
layout( push_constant ) uniform MeshObjectPushConstant
{
    int albedoTextureID;
    int normalTextureID;
    int additionalTextureID;
    int additionalTextureID2;
} objectPushConstant;

//OUTPUTS-------------------------
layout(location = 0) out vec4 outColor;

//GLOBALS
layout(binding = 1) uniform sampler2D texSampler[16];

void main() {
    outColor = texture(texSampler[objectPushConstant.albedoTextureID], fragTexUV);
    //outColor = fragColor;
}
