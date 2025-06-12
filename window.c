#include "window.h"

struct winnode {
  bool newframe;
  struct window *next;
};

struct windows {
  struct window *first;
};

struct windows *WayVR_WinList_Init() {
  struct windows *WinList = SDL_malloc(sizeof(struct windows));
  WinList->first = NULL;
  return WinList;
}

void WayVR_WinList_Deinit(struct windows *WinList) {
  SDL_free(WinList);
}

bool WayVR_WinList_Add(struct windows *WinList, struct window win) {
  for (struct window *it = WinList->first; it != NULL; it = it->internal->next)
    if (!SDL_memcmp(it->id.data, win.id.data, sizeof(SDL_GUID)))
      return false;

  SDL_GUID g = win.id;
  SDL_Log("GUID: %x%x%x%x%x%x%x%x", g.data[0], g.data[1], g.data[2], g.data[3], g.data[4], g.data[5], g.data[6], g.data[7]);
  SDL_Log("GUID: %x%x%x%x%x%x%x%x", g.data[8], g.data[9], g.data[10], g.data[11], g.data[12], g.data[13], g.data[14], g.data[15]);

  struct window *newwin = SDL_malloc(sizeof(struct window));
  *newwin = win;
  newwin->internal = SDL_malloc(sizeof(struct winnode));
  newwin->internal->next = NULL;
  newwin->internal->newframe = false;

  struct window *last = WinList->first;
  if (last == NULL)
    WinList->first = newwin;
  else {
    while (last->internal->next != NULL)
      last = last->internal->next;
    last->internal->next = newwin;
  }
  return true;
}

struct window *WayVR_WinList_GetNext(struct windows *d, struct window *n) {
  if (n == NULL)
    return d->first;
  else
    return n->internal->next;
}

void WayVR_Init(struct window *win) {
  win->internal = SDL_malloc(sizeof(struct winnode));
  win->internal->newframe = false;
  /*switch (win->type) {
    case WAYVR_WINDOWTYPE_CAM2D:
      return WayVR_Cam2D_Init(win->win.win2d);
    case WAYVR_WINDOWTYPE_NONE:
      SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "WayVR_Deinit called on WAYVR_WINDOWTYPE_NONE");
      return;
  }
  SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "WayVR_Deinit called on Unknown WINDOWTYPE");*/
}

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
  bool ret = false;
  switch (win->type) {
    case WAYVR_WINDOWTYPE_CAM2D:
      ret = WayVR_Cam2D_AcquireFrame(win->win.win2d);
      break;
    case WAYVR_WINDOWTYPE_NONE:
      SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "WayVR_AcquireFrame called on WAYVR_WINDOWTYPE_NONE");
      break;
    default:
      SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "WayVR_AcquireFrame called on Unknown WINDOWTYPE");
      break;
  }
  win->internal->newframe = ret;
  return ret;
}

void WayVR_UploadFrame(struct window *win, SDL_GPUCopyPass *CopyPass) {
  if (!win->internal->newframe)
    return;

  switch (win->type) {
    case WAYVR_WINDOWTYPE_CAM2D:
      WayVR_Cam2D_UploadFrame(win->win.win2d, CopyPass);
      break;
    case WAYVR_WINDOWTYPE_NONE:
      SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "WayVR_UploadFrame called on WAYVR_WINDOWTYPE_NONE");
      break;
    default:
      SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "WayVR_UploadFrame called on Unknown WINDOWTYPE");
      break;
  }
}

SDL_GPUTexture *WayVR_GetTexture(struct window *win) {
  return WayVR_Cam2D_GetTexture(win->win.win2d);
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

