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

struct win2d *InitWin2D(SDL_CameraID camid, SDL_GPUDevice *dev);
void DeinitWin2D(struct win2d *win);
bool AcquireFrame(struct win2d *win);
SDL_GPUTexture *UploadFrame(struct win2d *win, SDL_GPUCopyPass *CopyPass);
struct Transform *GetTransform(struct win2d *win);
void CalculateModelMat(struct win2d *win, mat4 model);

#endif
