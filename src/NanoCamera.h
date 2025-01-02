#ifndef NANOCAMERA_H_
#define NANOCAMERA_H_

#include "cglm/cglm.h"

typedef struct NanoCamera NanoCamera;
typedef struct CameraParam CameraParam;

struct RenderableNode;

struct NanoCamera{
    vec3 position;
    vec3 lookAt;

    float fov; // in degrees
    float aspectRatio; // width / height

    mat4 view;
    mat4 proj;

    struct RenderableNode* objectTracked;
    void (*Update)(void* cameraToUpdate, void* frameData);
};

struct CameraParam{
    vec3 position;
    vec3 lookAt;
    float fov;
    float aspectRatio;
};

void InitCamera(NanoCamera* camera, CameraParam* param);
NanoCamera CreateCamera(CameraParam param);
void CopyCamera(NanoCamera* cameraSrc, NanoCamera* cameraDest);

#endif // NANOCAMERA_H_
