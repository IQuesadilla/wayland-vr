#include <SDL3/SDL.h>
#include "displays.h"

struct display {
  char name[64];
  SDL_GPUGraphicsPipeline *pipeline;
  SDL_GPUTexture *depthTexture;
  SDL_Window *win;
  struct Transform t;
  struct display *next;
};

struct displays {
  SDL_GPUDevice *gpudev;
  SDL_GPUGraphicsPipelineCreateInfo pipelineci;
  struct display *list;
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
  for (struct display *it = d->list; it != NULL; it = it->next) {
    SDL_ReleaseWindowFromGPUDevice(d->gpudev, it->win);
    SDL_DestroyWindow(it->win);
  }
}

struct display *GetNextDisplay(struct displays *d, struct display *n) {
  if (n == NULL)
    return d->list;
  else
    return n->next;
}

struct Transform *GetDisplayTransform(struct display *n) {
  return &n->t;
}

SDL_GPURenderPass *BeginRenderPass(struct display *d, SDL_GPUCommandBuffer *cmdbuf) {
  bool s;
  SDL_GPUTexture *swapchainTexture;
  s = SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, d->win, &swapchainTexture, NULL, NULL);
  if (!s || swapchainTexture == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to acquire GPU swapchain texture (%s)", SDL_GetError());
    return NULL;
  }
  SDL_GPUColorTargetInfo ColorTargetInfo = {0};
  ColorTargetInfo.texture = swapchainTexture;
  ColorTargetInfo.clear_color = (SDL_FColor){0.5f, 0.5f, 0.5f, 1.f};
  ColorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
  ColorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

  SDL_GPUDepthStencilTargetInfo DepthTargetInfo = {0};
  DepthTargetInfo.texture = d->depthTexture;
  DepthTargetInfo.clear_depth = 1.f;
  DepthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
  DepthTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE;
  DepthTargetInfo.cycle = true;
  DepthTargetInfo.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
  DepthTargetInfo.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

  SDL_GPURenderPass *ret = SDL_BeginGPURenderPass(cmdbuf, &ColorTargetInfo, 1, &DepthTargetInfo);
  SDL_BindGPUGraphicsPipeline(ret, d->pipeline);
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

    struct display *last = NULL;
    bool found = false;
    for (struct display *it = disp->list; it != NULL; it = it->next) {
      if (SDL_strcmp(it->name, name) == 0) { // Display found in list
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
        .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .has_depth_stencil_target = true,
      };

      SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(disp->gpudev, &pipelineci);
      if (pipeline == NULL) {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create graphics pipeline for window %s (%s)", name, SDL_GetError());
        return false;
      }

      SDL_GPUTexture *depthTexture = SDL_CreateGPUTexture(
        disp->gpudev,
        &(SDL_GPUTextureCreateInfo) {
          .type = SDL_GPU_TEXTURETYPE_2D,
          .width = mode->w,
          .height = mode->h,
          .layer_count_or_depth = 1,
          .num_levels = 1,
          .sample_count = SDL_GPU_SAMPLECOUNT_1,
          .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
          .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET
        }
      );
      if (depthTexture == NULL) {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create depth texture for window %s (%s)", name, SDL_GetError());
        return false;
      }

      struct display *newnode = SDL_malloc(sizeof(struct display));
      newnode = SDL_malloc(sizeof(struct display));
      SDL_utf8strlcpy(newnode->name, name, 64);
      newnode->pipeline = pipeline;
      newnode->depthTexture = depthTexture;
      newnode->win = win;
      newnode->next = NULL;
      SDL_memset(&newnode->t, 0, sizeof(newnode->t));
      //newnode->t.Rotation[2] = 1.f; // Start looking at +z

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

void CalculateVPMatrix(struct display *d, mat4 view, mat4 persp) {
  //glm_mat4_identity(vp);
  int w, h;
  SDL_GetWindowSize(d->win, &w, &h);
  //SDL_Log("%f", ((float)w) / ((float)h));
  glm_perspective(70.f, ((float)w) / ((float)h), 0.1f, 30.f, persp);
  versor quat;
  glm_euler_yxz_quat(d->t.Rotation, quat);
  glm_quat_look(d->t.Translation, quat, view);
}
