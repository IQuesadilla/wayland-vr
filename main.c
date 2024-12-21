#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_net/SDL_net.h>

#include "console.h"

#define AssertExit(success,category,msg) if(!(((success)))){SDL_LogCritical(category, "%s (%s)", msg, SDL_GetError()); return SDL_APP_FAILURE;}

typedef struct {
  SDL_Window *win;
  SDL_Renderer *rend;
  Uint64 frametime;
  float loc;
  struct console *con;
} MyAppstate;

SDL_AppResult SDL_AppIterate(void *appstate) {
  MyAppstate *state = (MyAppstate*)appstate;

  Uint64 newframetime = SDL_GetTicksNS();
  Uint64 dT = newframetime - state->frametime;
  state->frametime = newframetime;

  state->loc += dT * 0.000000001f;
  while (state->loc > 1.f) state->loc -= 1.f;

  SDL_SetRenderDrawColor(state->rend, 0x99, 0, 0, 0xff);
  SDL_RenderClear(state->rend);
  console_draw(state->con);
  SDL_RenderPresent(state->rend);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *e) {
  MyAppstate *state = (MyAppstate*)appstate;
  char newline = '\n', tab = '\t';
  switch(e->type) {
    case SDL_EVENT_QUIT:
      return SDL_APP_SUCCESS;
      break;
    case SDL_EVENT_WINDOW_RESIZED: {
      int w, h;
      SDL_GetWindowSize(state->win, &w, &h);
      console_resize(state->con, w, h);
    } break;
    case SDL_EVENT_KEY_DOWN:
      switch(e->key.key) {
        case SDLK_ESCAPE:
          return SDL_APP_SUCCESS;
          break;
        case SDLK_RETURN: 
          console_append(state->con, &newline, 1);
          break;
        case SDLK_TAB:
          console_append(state->con, &tab, 1);
          break;
      }
      break;
    case SDL_EVENT_TEXT_INPUT:
      console_append(state->con, e->text.text, SDL_strlen(e->text.text));
      break;
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  MyAppstate *state = SDL_malloc(sizeof(MyAppstate));
  *appstate = state;
  bool s = true;

  s = SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "My App");
  AssertExit(s, SDL_LOG_CATEGORY_SYSTEM, "Setting App Name Metadata");

  s = SDL_InitSubSystem(SDL_INIT_EVENTS);
  AssertExit(s, SDL_LOG_CATEGORY_SYSTEM, "Events Init");

  s = SDL_InitSubSystem(SDL_INIT_VIDEO);
  AssertExit(s, SDL_LOG_CATEGORY_VIDEO, "Video Init");

  s = TTF_Init();
  AssertExit(s, SDL_LOG_CATEGORY_RENDER, "TTF Init");

  s = SDL_CreateWindowAndRenderer("hide", 1280, 1024, SDL_WINDOW_RESIZABLE | SDL_WINDOW_TRANSPARENT, &state->win, &state->rend);
  AssertExit(s, SDL_LOG_CATEGORY_VIDEO, "Creating Window and Renderer");

  s = console_init(&state->con, state->rend);

  state->frametime = SDL_GetTicksNS();
  state->loc = 0.f;
  
  SDL_StartTextInput(state->win);

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  MyAppstate *state = (MyAppstate*)appstate;
  SDL_StopTextInput(state->win);
  console_deinit(state->con);
  SDL_DestroyRenderer(state->rend);
  SDL_DestroyWindow(state->win);
  TTF_Quit();
  SDL_Quit();
  return;
}
