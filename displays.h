#ifndef WAYVR_DISPLAYS_H
#define WAYVR_DISPLAYS_H

#include <SDL3/SDL.h>
#include <cglm/cglm.h>
#include <cam2d.h>

struct display;
struct displays;

struct displays* InitDisplays(SDL_GPUDevice *dev, SDL_GPUGraphicsPipelineCreateInfo pipeline);
void DeinitDisplays(struct displays *d);
struct display *GetNextDisplay(struct displays *d, struct display *n);
struct Transform *GetDisplayTransform(struct display *n);
SDL_GPURenderPass *BeginRenderPass(struct display *d, SDL_GPUCommandBuffer *cmdbuf);
bool UpdateDisplays(struct displays *disp, int *dispcount);
void CalculateVPMatrix(struct display *d, mat4 view, mat4 persp);

#endif
