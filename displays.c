#include <SDL3/SDL.h>
#include "displays.h"

struct display {
  char name[64];
  SDL_GPUGraphicsPipeline *pipeline;
  SDL_Window *win;
};

struct displays {
  SDL_GPUDevice *gpudev;
  SDL_GPUGraphicsPipelineCreateInfo pipelineci;
  struct displaynode *list;
};

struct displays* InitDisplays(SDL_GPUDevice *dev, SDL_GPUGraphicsPipelineCreateInfo pipeline) {
  struct displays *disp = SDL_malloc(sizeof(struct displays));
  disp->gpudev = dev;
  disp->list = NULL;
  //SDL_Window *win = SDL_CreateWindow(",,", 1280, 1024, SDL_WINDOW_RESIZABLE);
  disp->pipelineci = pipeline;
  return disp;
}

void DeinitDisplays(struct displays *d) {
  for (struct displaynode *it = d->list; it != NULL; it = it->next) {
    SDL_ReleaseWindowFromGPUDevice(d->gpudev, it->d->win);
    SDL_DestroyWindow(it->d->win);
  }
}

struct displaynode *GetDisplayList(struct displays *d) {
  return d->list;
}

SDL_GPURenderPass *BeginRenderPass(struct displaynode *d, SDL_GPUCommandBuffer *cmdbuf) {
  bool s;
  SDL_GPUTexture *swapchainTexture;
  s = SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, d->d->win, &swapchainTexture, NULL, NULL);
  if (!s || swapchainTexture == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to acquire GPU swapchain texture (%s)", SDL_GetError());
    return NULL;
  }
  SDL_GPUColorTargetInfo ColorTargetInfo = {0};
  ColorTargetInfo.texture = swapchainTexture;
  ColorTargetInfo.clear_color = (SDL_FColor){0.5f, 0.5f, 0.5f, 1.f};
  ColorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
  ColorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

  SDL_GPURenderPass *ret = SDL_BeginGPURenderPass(cmdbuf, &ColorTargetInfo, 1, NULL);
  SDL_BindGPUGraphicsPipeline(ret, d->d->pipeline);
  return ret;
}

bool UpdateDisplays(struct displays *disp, int *dispcount) {
  bool s;
  SDL_DisplayID *disps = SDL_GetDisplays(dispcount); // No hotplug support...?
  if (disps == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "Failed to get display list - %s", SDL_GetError());
    return false;
  }

  for (int k = 0; k < *dispcount; ++k) {
    const char *name = SDL_GetDisplayName(disps[k]);
    if (name == NULL) {
      SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "Failed to get display name - %s", SDL_GetError());
      return false;
    }

    struct displaynode *last = NULL;
    bool found = false;
    for (struct displaynode *it = disp->list; it != NULL; it = it->next) {
      if (SDL_strcmp(it->d->name, name) == 0) { // Display found in list
        found = true;
        break;
      }
      last = it;
    }

    if (!found) {
      //SDL_Rect bounds = {0};
      //SDL_GetDisplayUsableBounds(disps[k], &bounds);
      const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(disps[k]);

      SDL_Window *win = SDL_CreateWindow(",,", mode->w, mode->h, 0);
      if (win == NULL) {
        SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "Failed to create window for window %s (%s)", name, SDL_GetError());
        return false;
      }
      SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED_DISPLAY(k), SDL_WINDOWPOS_CENTERED_DISPLAY(k));
      s = SDL_SetWindowFullscreenMode(win, mode);
      if (!s) {
        SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "Failed to set fullscreen mode for window %s (%s)", name, SDL_GetError());
        return false;
      }
      SDL_SetWindowFullscreen(win, true);

      s = SDL_ClaimWindowForGPUDevice(disp->gpudev, win);
      if (!s) {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to enable GPU for window %s (%s)", name, SDL_GetError());
        return false;
      }

      SDL_GPUGraphicsPipelineCreateInfo pipelineci = disp->pipelineci;
      pipelineci.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
        .num_color_targets = 1,
        .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
          .format = SDL_GetGPUSwapchainTextureFormat(disp->gpudev, win)
        }},
      };

      SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(disp->gpudev, &pipelineci);
      if (pipeline == NULL) {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create graphics pipeline for window %s (%s)", name, SDL_GetError());
        return false;
      }

      struct displaynode *newnode = SDL_malloc(sizeof(struct displaynode));
      newnode->d = SDL_malloc(sizeof(struct display));
      SDL_utf8strlcpy(newnode->d->name, name, 64);
      newnode->d->pipeline = pipeline;
      newnode->d->win = win;
      newnode->next = NULL;
      SDL_memset(&newnode->t, 0, sizeof(newnode->t));

      SDL_Log("%d: '%s' %dx%d@%.2f",
              disps[k],
              name,
              mode->w,
              mode->h,
              mode->refresh_rate);

      if (last == NULL) disp->list = newnode;
      else              last->next = newnode;
    }
  }
  SDL_free(disps);
  return true;
}

void CalculateVPMatrix(struct displaynode *d, mat4 view, mat4 persp) {
  //glm_mat4_identity(vp);
  int w, h;
  SDL_GetWindowSize(d->d->win, &w, &h);
  //SDL_Log("%f", ((float)w) / ((float)h));
  glm_perspective(70.f, ((float)w) / ((float)h), 0.1f, 30.f, persp);
  glm_look((vec3){0.f, 0.f, 10.f}, (vec3){0.f, 0.f, -1.f}, (vec3){0.f, 1.f, 0.f}, view);
}
