#version 450

#define MAX_TEXTURES_PER_SCENE 16

//INPUTS--------------------------
layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexUV;
layout(location = 3) in vec3 fragLightPos;
layout(location = 4) in vec3 fragPos;

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
layout(binding = 2) uniform sampler2D textureSampler[MAX_TEXTURES_PER_SCENE];

void main() {

    vec4 colorToUse = fragColor;
    if(objectPushConstant.albedoTextureID >= 0){
        colorToUse = texture(textureSampler[objectPushConstant.albedoTextureID], fragTexUV);
    }

    vec3 lightDirection = normalize(fragLightPos - fragPos);
    vec3 viewDirection = normalize(-fragPos );
    vec3 halfVector = normalize( lightDirection + viewDirection);

    float diffuse = max(0.0f,dot( fragNormal.xyz, lightDirection));
    float specular = max(0.0f,dot( fragNormal.xyz, halfVector ) );
    float distanceFromLight = length(fragLightPos - fragPos);
    float distanceFromfragment = length(fragPos);
    if (diffuse == 0.0) {
        specular = 0.0;
    } else {
        specular = pow( specular, 64.0f );
    }

    vec3 scatteredLight = 1.0f/distanceFromLight * colorToUse.xyz * diffuse * 10.0f;
    vec3 reflectedLight = vec3(1.0f,1.0f,1.0f) * specular;
    vec3 ambientLight = colorToUse.xyz * 0.01f * 40.0f/distanceFromLight;

    outColor = vec4(min( ambientLight + scatteredLight + reflectedLight, vec3(1,1,1)), 1.0);

}
