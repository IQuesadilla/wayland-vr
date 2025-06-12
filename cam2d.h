#ifndef WAYVR_CAM2D_H
#define WAYVR_CAM2D_H
#include <SDL3/SDL.h>
#include <cglm/cglm.h>

struct Transform {
  vec3 Translation;
  vec3 Rotation;
  vec3 Scale;
};

struct cam2d;

struct cam2d *WayVR_Cam2D_Init(SDL_CameraID camid, SDL_GPUDevice *dev);
void WayVR_Cam2D_Deinit(struct cam2d *cam);
bool WayVR_Cam2D_AcquireFrame(struct cam2d *cam);
void WayVR_Cam2D_UploadFrame(struct cam2d *cam, SDL_GPUCopyPass *CopyPass);
SDL_GPUTexture *WayVR_Cam2D_GetTexture(struct cam2d *cam);
struct Transform *WayVR_Cam2D_GetTransform(struct cam2d *cam);
void WayVR_Cam2D_CalculateModelMat(struct cam2d *cam, mat4 model);

#endif
