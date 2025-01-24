#ifndef WAYVR_DISPLAYS_H
#define WAYVR_DISPLAYS_H

#include <SDL3/SDL.h>
#include <cglm/cglm.h>
#include <win2d.h>

struct display;
struct displays;

struct displaynode {
  struct display *d;
  struct Transform t;
  struct displaynode *next;
  void *user;
};

struct displays* InitDisplays(SDL_GPUDevice *dev, SDL_GPUGraphicsPipelineCreateInfo pipeline);
void DeinitDisplays(struct displays *d);
struct displaynode *GetDisplayList(struct displays *d);
SDL_GPURenderPass *BeginRenderPass(struct displaynode *d, SDL_GPUCommandBuffer *cmdbuf);
bool UpdateDisplays(struct displays *disp, int *dispcount);
void CalculateVPMatrix(struct displaynode *d, mat4 view, mat4 persp);

#endif
