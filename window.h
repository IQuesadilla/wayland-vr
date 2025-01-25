#ifndef WAYVR_WINDOW_H
#define WAYVR_WINDOW_H

#include "win2d.h"

enum WindowType {
  WAYVR_WINDOWTYPE_NONE,
  WAYVR_WINDOWTYPE_CAM2D,
};

union WindowPtr {
  struct win2d *win2d;
};

struct window {
  enum WindowType type;
  union WindowPtr win;
};

//struct win2d *WayVR_Init(SDL_CameraID camid, SDL_GPUDevice *dev);
void WayVR_Deinit(struct window *win);
bool WayVR_AcquireFrame(struct window *win);
SDL_GPUTexture *WayVR_UploadFrame(struct window *win, SDL_GPUCopyPass *CopyPass);
struct Transform *WayVR_GetTransform(struct window *win);
void WayVR_CalculateModelMat(struct window *win, mat4 model);

#endif
