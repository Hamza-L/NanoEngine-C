#include "NanoCamera.h"

#include "NanoConfig.h"
#include "cglm/clipspace/persp_rh_zo.h"
#include "cglm/clipspace/view_rh_zo.h"
#include "NanoScene.h"

static void UpdateCamera(void* cameraToUpdate, void* frameData);

static CameraParam defaultParam = {
.position = {5.0, 5.0, 5.0},
.lookAt = {0.0f, 0.0f, 0.0f},
.aspectRatio = 800.0f/600.0f,
.fov = 50,
};

void UpdateCamera(void* cameraToUpdate, void* frameData){
    NanoCamera* camera = (NanoCamera*)cameraToUpdate;

    if (camera->objectTracked){
        glm_vec3_copy(camera->objectTracked->localModel[3], camera->lookAt);
    }

    vec3 up = {0.0f, 1.0f, 0.0f};
    glm_lookat_rh_zo(camera->position, camera->lookAt, up, camera->view);

    // Projection matrix
    glm_perspective_rh_zo(glm_rad(camera->fov), camera->aspectRatio , 0.1f, 100.0f, camera->proj);
}

void InitCamera(NanoCamera* camera, CameraParam* param /* width/height */){

    CameraParam camParam;
    if (param == nullptr)
        camParam = defaultParam;
    else
        camParam = *param;

    // Camera info
    glm_vec3_copy(camParam.position, camera->position);
    glm_vec3_copy(camParam.lookAt, camera->lookAt);
    camera->aspectRatio = camParam.aspectRatio;
    camera->fov = camParam.fov;
    camera->Update = UpdateCamera;
    camera->objectTracked = nullptr;

    // View matrix
    vec3 up = {0.0f, 1.0f, 0.0f};
    glm_lookat_rh_zo(camera->position, camera->lookAt, up, camera->view);

    // Projection matrix
    glm_perspective_rh_zo(glm_rad(camera->fov), camera->aspectRatio , 0.1f, 100.0f, camera->proj);
}

NanoCamera CreateCamera(CameraParam camParam){
    NanoCamera camera = {};
    InitCamera(&camera, &camParam);
    return camera;
}
