#ifndef WAYVR_WIN2D_H
#define WAYVR_WIN2D_H
#include <SDL3/SDL.h>
#include <cglm/cglm.h>

struct Transform {
  vec3 Translation;
  vec3 Rotation;
  vec3 Scale;
};

struct win2d;

struct win2d *WayVR_Cam2D_Init(SDL_CameraID camid, SDL_GPUDevice *dev);
void WayVR_Cam2D_Deinit(struct win2d *win);
bool WayVR_Cam2D_AcquireFrame(struct win2d *win);
SDL_GPUTexture *WayVR_Cam2D_UploadFrame(struct win2d *win, SDL_GPUCopyPass *CopyPass);
struct Transform *WayVR_Cam2D_GetTransform(struct win2d *win);
void WayVR_Cam2D_CalculateModelMat(struct win2d *win, mat4 model);

#endif
