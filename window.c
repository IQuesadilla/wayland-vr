#include "window.h"

void WayVR_Deinit(struct window *win) {
  switch (win->type) {
    case WAYVR_WINDOWTYPE_CAM2D:
      return WayVR_Cam2D_Deinit(win->win.win2d);
    case WAYVR_WINDOWTYPE_NONE:
      SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "WayVR_Deinit called on WAYVR_WINDOWTYPE_NONE");
      return;
  }
  SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "WayVR_Deinit called on Unknown WINDOWTYPE");
}

bool WayVR_AcquireFrame(struct window *win) {
  switch (win->type) {
    case WAYVR_WINDOWTYPE_CAM2D:
      return WayVR_Cam2D_AcquireFrame(win->win.win2d);
    case WAYVR_WINDOWTYPE_NONE:
      SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "WayVR_AcquireFrame called on WAYVR_WINDOWTYPE_NONE");
      return false;
  }
  SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "WayVR_AcquireFrame called on Unknown WINDOWTYPE");
  return false;
}

SDL_GPUTexture *WayVR_UploadFrame(struct window *win, SDL_GPUCopyPass *CopyPass) {
  switch (win->type) {
    case WAYVR_WINDOWTYPE_CAM2D:
      return WayVR_Cam2D_UploadFrame(win->win.win2d, CopyPass);
    case WAYVR_WINDOWTYPE_NONE:
      SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "WayVR_UploadFrame called on WAYVR_WINDOWTYPE_NONE");
      return NULL;
  }
  SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "WayVR_UploadFrame called on Unknown WINDOWTYPE");
  return NULL;
}

struct Transform *WayVR_GetTransform(struct window *win) {
  switch (win->type) {
    case WAYVR_WINDOWTYPE_CAM2D:
      return WayVR_Cam2D_GetTransform(win->win.win2d);
    case WAYVR_WINDOWTYPE_NONE:
      SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "WayVR_GetTransform called on WAYVR_WINDOWTYPE_NONE");
      return NULL;
  }
  SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "WayVR_GetTransform called on Unknown WINDOWTYPE");
  return NULL;
}

void WayVR_CalculateModelMat(struct window *win, mat4 model) {
  switch (win->type) {
    case WAYVR_WINDOWTYPE_CAM2D:
      return WayVR_Cam2D_CalculateModelMat(win->win.win2d, model);
    case WAYVR_WINDOWTYPE_NONE:
      SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "WayVR_CalculateModelMat called on WAYVR_WINDOWTYPE_NONE");
      return;
  }
  SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "WayVR_CalculateModelMat called on Unknown WINDOWTYPE");
}

