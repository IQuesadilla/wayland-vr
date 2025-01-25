#include "wayvr.h"

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *e) {
  struct WayVR_Appstate *app = (struct WayVR_Appstate*)appstate;
  switch (e->type) {
    case SDL_EVENT_QUIT:
      return SDL_APP_SUCCESS;
      break;
    case SDL_EVENT_KEY_DOWN:
      switch (e->key.key) {
        case SDLK_ESCAPE:
          SDL_Log("Found Esc!");
          return SDL_APP_SUCCESS;
          break;
      }
      break;
    case SDL_EVENT_CAMERA_DEVICE_ADDED:
      if (app->win->type == WAYVR_WINDOWTYPE_NONE) {
        SDL_Log("Found new camera!");
        app->win->win.win2d = WayVR_Cam2D_Init(e->cdevice.which, app->dev);
        app->win->type = WAYVR_WINDOWTYPE_CAM2D;
      } else {
        SDL_Log("One camera is already connected");
      }
      break;
    case SDL_EVENT_CAMERA_DEVICE_REMOVED:
      SDL_Log("Removed camera!");
      WayVR_Deinit(app->win);
      app->win->type = WAYVR_WINDOWTYPE_NONE;
      break;
    case SDL_EVENT_GAMEPAD_ADDED:
      if (app->curgpad == NULL) {
        SDL_Log("Found new gamepad!");
        app->curgpad = SDL_OpenGamepad(e->gdevice.which);
      } else {
        SDL_Log("One gamepad already connected");
      }
      break;
    case SDL_EVENT_GAMEPAD_REMOVED:
      SDL_Log("Removed gamepad!");
      SDL_CloseGamepad(app->curgpad);
      app->curgpad = NULL;
      break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
      switch (e->gbutton.button) {
        case SDL_GAMEPAD_BUTTON_START:
          SDL_Log("Found Start!");
          return SDL_APP_SUCCESS;
          break;
        case SDL_GAMEPAD_BUTTON_SOUTH:
          app->SelectWin = true;
          break;
        case SDL_GAMEPAD_BUTTON_EAST:
          app->cwin = NULL;
          break;
        default:
          SDL_Log("Gamepad button down: %s", SDL_GetGamepadStringForButton(e->gbutton.button));
          break;
      }
      break;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
      switch (e->gaxis.axis) {
        case SDL_GAMEPAD_AXIS_LEFTX:
          if (abs(e->gaxis.value) > app->Config.Deadzone)
            app->Movement.Translation[0] = (((float)e->gaxis.value) / 32768.f) * app->Config.MovementSpeed;
          else app->Movement.Translation[0] = 0;
          break;
        case SDL_GAMEPAD_AXIS_LEFTY:
          if (abs(e->gaxis.value) > app->Config.Deadzone)
            app->Movement.Translation[2] = (((float)e->gaxis.value) / 32768.f) * app->Config.MovementSpeed;
          else app->Movement.Translation[2] = 0;
          break;
        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
          if (abs(e->gaxis.value) > app->Config.Deadzone)
            app->Movement.Translation[1] = (((float)e->gaxis.value) / -32768.f) * app->Config.MovementSpeed * cosf(app->Movement.Rotation[0]) * cosf(app->Movement.Rotation[1]);
          else app->Movement.Translation[1] = 0;
          break;
        case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
          if (abs(e->gaxis.value) > app->Config.Deadzone)
            app->Movement.Translation[1] = (((float)e->gaxis.value) / 32768.f) * app->Config.MovementSpeed;
          else app->Movement.Translation[1] = 0;
          break;
        case SDL_GAMEPAD_AXIS_RIGHTX:
          if (abs(e->gaxis.value) > app->Config.Deadzone)
            app->Movement.Rotation[1] = (((float)e->gaxis.value) / -32768.f) * app->Config.RotationSpeed;
          else app->Movement.Rotation[1] = 0;
        break;
        case SDL_GAMEPAD_AXIS_RIGHTY:
          if (abs(e->gaxis.value) > app->Config.Deadzone)
            app->Movement.Rotation[0] = (((float)e->gaxis.value) / -32768.f) * app->Config.RotationSpeed;
          else app->Movement.Rotation[0] = 0;
          break;
      }
      break;
  }
  return SDL_APP_CONTINUE;
}
