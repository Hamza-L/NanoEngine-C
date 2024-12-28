#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout (binding = 1) uniform UboInstance {
	mat4 model;
} uboInstance;

//It is important to know that some types, like dvec3 64 bit vectors, use multiple slots. That means that the index after it must be at least 2 higher
//INPUTS--------------------------
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexUV;


layout(constant_id = 0) const int VERT_CONSTANT = 0;

//OUTPUTS-------------------------
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexUV;

void main() {
    mat4 modelMat = {
    {1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f}};
    gl_Position = ubo.proj * ubo.view * uboInstance.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexUV = inTexUV;
}
