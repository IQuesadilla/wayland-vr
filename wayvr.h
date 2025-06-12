#ifndef WAYVR_MAIN_H
#define WAYVR_MAIN_H

#include <SDL3/SDL.h>
#include "displays.h"
#include "window.h"
#include <cglm/cglm.h>

static const float I = 1.f;
static const SDL_FPoint Vertices[] = {
  {-I,  I},
  { I,  I},
  {-I, -I},
  {-I, -I},
  { I, -I},
  { I,  I},
};
static const SDL_FPoint UVs[] = {
  {0.f, 0.f},
  {1.f, 0.f},
  {0.f, 1.f},
  {0.f, 1.f},
  {1.f, 1.f},
  {1.f, 0.f},
};

struct WayVR_Configuration {
  float MovementSpeed;
  float RotationSpeed;
  int Deadzone;
};
static const struct WayVR_Configuration DefaultConfiguraion = {
  .MovementSpeed = 0.1f,
  .RotationSpeed = 0.05f,
  .Deadzone = 10000
};

struct WayVR_Appstate {
  struct WayVR_Configuration Config;
  SDL_GPUDevice *dev;
  struct windows *WinList; // List of all windows - TODO
  struct window *cwin; // Currently selected window
  bool SelectWin;
  SDL_Gamepad *curgpad; // TODO Allow many inputs
  struct Transform Movement; // Represents the user inputs as a transform
  struct displays *DisplayList;
  SDL_GPUBufferBinding vertBinding;
  SDL_GPUTextureSamplerBinding WallpaperSampler, WindowSampler;
};

#endif
