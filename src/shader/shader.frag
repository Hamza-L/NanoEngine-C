#version 450

#define MAX_TEXTURES_PER_SCENE 16

//INPUTS--------------------------
layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexUV;

// push constants
layout( push_constant ) uniform MeshObjectPushConstant
{
    int albedoTextureID;
    int normalTextureID;
    int additionalTextureID;
    int additionalTextureID2;
} objectPushConstant;

// specialization constants
layout(constant_id = 1) const int NUM_TEXTURES = 0;

//OUTPUTS-------------------------
layout(location = 0) out vec4 outColor;

//GLOBALS
// layout(binding = 2) uniform sampler2D textSampler;
layout(binding = 1) uniform sampler2D textureSampler[MAX_TEXTURES_PER_SCENE];

void main() {
    if(objectPushConstant.albedoTextureID >= 0)
        outColor = texture(textureSampler[objectPushConstant.albedoTextureID], fragTexUV);
    else
        outColor = fragColor;
}
