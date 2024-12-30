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
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inTexUV;


layout(constant_id = 0) const int VERT_CONSTANT = 0;

//OUTPUTS-------------------------
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexUV;
layout(location = 3) out vec3 fragLightPos;
layout(location = 4) out vec3 fragPos;


mat3 adjugate(in mat3 m) {
    return mat3(
        cross(m[1], m[2]),
        cross(m[2], m[0]),
        cross(m[0], m[1])
    );
}

void main() {
    gl_Position = ubo.proj * ubo.view * uboInstance.model * vec4(inPosition, 1.0);

    vec4 tempPos = ubo.view * uboInstance.model * vec4(inPosition, 1.0);
    fragPos = tempPos.xyz;

    fragColor = inColor;
    fragTexUV = inTexUV;
    fragNormal = normalize(adjugate(mat3(uboInstance.model)) * inNormal);

    fragLightPos = mat3(ubo.view) * vec3(4.0f,4.0f,4.0f);;
}
