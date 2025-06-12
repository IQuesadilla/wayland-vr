#ifndef WAYVR_WINDOW_H
#define WAYVR_WINDOW_H

#include "cam2d.h"

enum WindowType {
  WAYVR_WINDOWTYPE_NONE,
  WAYVR_WINDOWTYPE_CAM2D,
};

union WindowPtr {
  struct cam2d *win2d;
};

struct window {
  enum WindowType type;
  union WindowPtr win;
  SDL_GUID id;
  struct winnode *internal;
};

struct windows;

struct windows *WayVR_WinList_Init();
void WayVR_WinList_Deinit(struct windows *WinList);
bool WayVR_WinList_Add(struct windows *WinList, struct window win);
struct window *WayVR_WinList_GetNext(struct windows *d, struct window *n);

void WayVR_Init(struct window *win);
void WayVR_Deinit(struct window *win);
bool WayVR_AcquireFrame(struct window *win);
void WayVR_UploadFrame(struct window *win, SDL_GPUCopyPass *CopyPass);
SDL_GPUTexture *WayVR_GetTexture(struct window *win);
struct Transform *WayVR_GetTransform(struct window *win);
void WayVR_CalculateModelMat(struct window *win, mat4 model);

#endif
