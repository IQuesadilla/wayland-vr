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
  SDL_Texture *winbuf;
  Uint64 frametime;
  float loc;
  TTF_TextEngine *tengine;
  TTF_Font *font;
  TTF_Text *text;
  SDL_Point CharSize;
  struct console *con;
} MyAppstate;

bool setfontsize(MyAppstate *state, int sz) {
  TTF_Font *newfont = TTF_OpenFont(WAYVR_ASSET_DIR "/assets/font.ttf", sz);
  if (newfont == NULL) return false;

  if (!TTF_FontIsFixedWidth(newfont)) {
    SDL_LogError(SDL_LOG_CATEGORY_RENDER, "FONT IS NOT MONOSPACE!\n");
    TTF_CloseFont(newfont);
    state->font = NULL;
    return false;
  }
  state->font = newfont;

  SDL_LogInfo(SDL_LOG_CATEGORY_INPUT, "Font is %s\n", TTF_GetFontFamilyName(state->font));

  int minx, maxx, miny, maxy, adv;
  TTF_GetGlyphMetrics(state->font, 'm', &minx, &maxx, &miny, &maxy, &adv);
  SDL_Log("MINX: %d | MAXX: %d | MINY: %d | MAXY: %d | ADV: %d\n", minx, maxx, miny, maxy, adv);

  TTF_GetStringSize(state->font, "m", 1, &state->CharSize.x, &state->CharSize.y);
  SDL_Log("W: %d, H: %d\n", state->CharSize.x, state->CharSize.y);
  return true;
}

void onresize(MyAppstate *state) {
  int w, h;
  SDL_GetWindowSize(state->win, &w, &h);
  SDL_Log("WINDOW RESIZED %d %d", w, h);
  if (state->winbuf != NULL)
    SDL_DestroyTexture(state->winbuf);
  state->winbuf = SDL_CreateTexture(state->rend, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
  //int fontw, fonth;
  //TTF_GetStringSize(state->font, "m", 1, &fontw, &fonth);
  //state->CharSize = (SDL_Point){.x = w / fontw, .y = h / fonth};
  //SDL_Log("New Char Size: %d %d", state->CharSize.x, state->CharSize.y);
  //TTF_SetTextWrapWidth(state->text, w);
  console_resize(state->con, (SDL_Point){.x = w / state->CharSize.x, .y = h / state->CharSize.y});
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  MyAppstate *state = (MyAppstate*)appstate;

  Uint64 newframetime = SDL_GetTicksNS();
  Uint64 dT = newframetime - state->frametime;
  state->frametime = newframetime;

  state->loc += dT * 0.000000001f;
  while (state->loc > 1.f) state->loc -= 1.f;

  struct console_updatelist *it = console_update(state->con);
  bool doRedraw = (it != NULL);
  while (it != NULL) { // TODO: Move to own function and benchmark
    SDL_SetRenderTarget(state->rend, state->winbuf);

    /*if (doRedraw > 1) {
      SDL_SetRenderDrawColor(state->rend, 0, 0, 0, 0xff);
      SDL_RenderClear(state->rend);
    }*/

    //TTF_AppendTextString(state->text, text, len);
    if (it->Length > 0) {
      /*char *debuglog = SDL_malloc(it->Length * 3 + 1);
      for (int k = 0; k < it->Length; ++k)
        SDL_snprintf(&debuglog[k*3], 4, "%02x ", it->Text[k]);
      debuglog[it->Length * 3 + 1] = 0;
      SDL_Log("t: %s %d", debuglog, it->Length);
      SDL_free(debuglog);*/
      TTF_SetTextString(state->text, (char*)it->Text, it->Length);// * sizeof(*it->Text));

      //console_draw(state->con);
      // NOTE: In this case, the dirty rectangles should refer to locations, not pixels
      /*int th, wh, y = 0;
      SDL_GetWindowSize(state->win, NULL, &wh);
      TTF_GetTextSize(state->text, NULL, &th);
      if (th > wh)
        y = wh - th;*/

      int x = state->CharSize.x * it->Location.x;
      int y = state->CharSize.y * it->Location.y;
      //SDL_Log("Drawing %d %d | %d %d | %d %d", x, y, state->CharSize.x, state->CharSize.y, it->Location.x, it->Location.y);
      TTF_DrawRendererText(state->text, x, y);
    }

    struct console_updatelist *temp = it;
    it = it->next;
    SDL_free(temp->Text);
    SDL_free(temp);
  }

  if (doRedraw) {
    SDL_SetRenderTarget(state->rend, NULL);
    //Uint64 t2u = SDL_GetTicksNS() - newframetime;
    //SDL_Log("Time to Update: %lu", t2u);
    //} // TODO: Commented to see if render updates are required for every frame

    SDL_SetRenderDrawColor(state->rend, 0, 0, 0, 255);
    SDL_RenderClear(state->rend);
    SDL_RenderTexture(state->rend, state->winbuf, NULL, NULL);
    SDL_RenderPresent(state->rend);
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *e) {
  MyAppstate *state = (MyAppstate*)appstate;
  char newline = '\n', tab = '\t';
  switch(e->type) {
    case SDL_EVENT_QUIT:
      return SDL_APP_SUCCESS;
      break;
    case SDL_EVENT_WINDOW_RESIZED:
      onresize(state);
      break;
    case SDL_EVENT_KEY_DOWN:
      switch(e->key.key) {
        case SDLK_ESCAPE:
          return SDL_APP_SUCCESS;
          break;
        case SDLK_RETURN: 
          console_write(state->con, &newline, sizeof(newline));
          break;
        case SDLK_TAB:
          console_write(state->con, &tab, sizeof(tab));
          break;
      }
      break;
    case SDL_EVENT_TEXT_INPUT:
      console_write(state->con, e->text.text, SDL_strlen(e->text.text));
      break;
  }
  return SDL_APP_CONTINUE; //SDL_AppIterate(appstate);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  MyAppstate *state = SDL_malloc(sizeof(MyAppstate));
  *appstate = state;
  bool s = true;

  SDL_Log("Video Driver List:");
  int numvd = SDL_GetNumVideoDrivers();
  for (int k = 0; k < numvd; ++k)
    SDL_Log("-> %s", SDL_GetVideoDriver(k));

  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland");

  s = SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "My App");
  AssertExit(s, SDL_LOG_CATEGORY_SYSTEM, "Setting App Name Metadata");

  s = SDL_InitSubSystem(SDL_INIT_EVENTS);
  AssertExit(s, SDL_LOG_CATEGORY_SYSTEM, "Events Init");

  s = SDL_InitSubSystem(SDL_INIT_VIDEO);
  AssertExit(s, SDL_LOG_CATEGORY_VIDEO, "Video Init");

  s = TTF_Init();
  AssertExit(s, SDL_LOG_CATEGORY_RENDER, "TTF Init");

  s = SDL_CreateWindowAndRenderer("hide", 1280, 1024, SDL_WINDOW_RESIZABLE, &state->win, &state->rend);
  AssertExit(s, SDL_LOG_CATEGORY_VIDEO, "Creating Window and Renderer");

  state->winbuf = NULL;
  //state->winbuf = SDL_CreateTexture(state->rend, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 1280, 1024);
  //AssertExit(state->winbuf != NULL, SDL_LOG_CATEGORY_RENDER, "Off-Screen Buffer");

  s = console_init(&state->con, state->rend);
  AssertExit(s, SDL_LOG_CATEGORY_RENDER, "Console Init");

  state->tengine = TTF_CreateRendererTextEngine(state->rend);
  if (state->tengine == NULL) return false;

  s = setfontsize(state, 16);
  if (state->font == NULL || !s) return false;

  onresize(state);

  state->text = TTF_CreateText(state->tengine, state->font, "", 0);
  if (state->text == NULL) return false;

  state->frametime = SDL_GetTicksNS();
  state->loc = 0.f;
  
  SDL_StartTextInput(state->win);

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  MyAppstate *state = (MyAppstate*)appstate;
  SDL_StopTextInput(state->win);
  console_deinit(state->con);
  TTF_DestroyText(state->text);
  TTF_CloseFont(state->font);
  TTF_DestroyRendererTextEngine(state->tengine); 
  SDL_DestroyTexture(state->winbuf);
  SDL_DestroyRenderer(state->rend);
  SDL_DestroyWindow(state->win);
  TTF_Quit();
  SDL_Quit();
  return;
}
