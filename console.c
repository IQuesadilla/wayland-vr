#include "console.h"

#include <SDL3_ttf/SDL_ttf.h>
#include <unistd.h>
#include <pty.h>
#include <signal.h>
#include <poll.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

void console_process(struct console *con, const char *text, int len);

/*------ Console Object Definition ------*/

struct console {
  TTF_TextEngine *tengine;
  TTF_Font *font;
  TTF_Text *text;
  int window_height;
  SDL_Process *proc;
  int confd;
  pid_t cpid;
  bool *SceneHasChanged;
  int curx, cury;
};

/*------ Console Object Functions ------*/

bool console_init(struct console **con, SDL_Renderer* rend, bool *SceneHasChanged) {
  struct console *newcon = (struct console*)SDL_malloc(sizeof(struct console));
  newcon->SceneHasChanged = SceneHasChanged;
  *newcon->SceneHasChanged = true;

  newcon->tengine = TTF_CreateRendererTextEngine(rend);
  if (newcon->tengine == NULL) return false;

  console_setfontsize(newcon, 12);
  if (newcon->font == NULL) return false;

  newcon->text = TTF_CreateText(newcon->tengine, newcon->font, "", 0);
  if (newcon->text == NULL) return false;

  int w, h;
  SDL_Window *win = SDL_GetRenderWindow(rend);
  SDL_GetWindowSize(win, &w, &h);
  console_resize(newcon, w, h);

  char* const args[] = {"/bin/sh", NULL};
  pid_t cpid = forkpty(&newcon->confd, NULL, NULL, NULL);
  if (cpid < 0) {
    SDL_Log("Failed to run forkpty!");
    return false;
  } else if (cpid == 0) {
    execv(args[0], args);
    return false;
  }
  newcon->cpid = cpid;
  newcon->curx = 0;
  newcon->cury = 0;

  *con = newcon;
  return true;
}

void console_deinit(struct console *con) {
  kill(con->cpid, SIGHUP);
  waitpid(con->cpid, NULL, 0);
  TTF_DestroyText(con->text);
  TTF_CloseFont(con->font);
  TTF_DestroyRendererTextEngine(con->tengine); 
}

void console_resize(struct console *con, int w, int h) {
  con->window_height = h;
  TTF_SetTextWrapWidth(con->text, w);

  int fontw, fonth;
  TTF_GetStringSize(con->font, "m", 1, &fontw, &fonth);
  struct winsize sz = {0};
  sz.ws_row = w / fontw;
  sz.ws_col = h / fonth;

  SDL_LogDebug(SDL_LOG_CATEGORY_RENDER, "Resize: w: %d, h: %d\n", sz.ws_col, sz.ws_row);
  ioctl(con->confd, TIOCSWINSZ, &sz);
}

void console_setfontsize(struct console *con, int sz) {
  TTF_Font *newfont = TTF_OpenFont("font.ttf", 16);
  if (newfont == NULL) return;

  if (!TTF_FontIsFixedWidth(newfont)) {
    SDL_LogError(SDL_LOG_CATEGORY_RENDER, "FONT IS NOT MONOSPACE!\n");
    TTF_CloseFont(newfont);
    con->font = NULL;
    return;
  }
  con->font = newfont;

  SDL_LogInfo(SDL_LOG_CATEGORY_INPUT, "Font is %s\n", TTF_GetFontFamilyName(con->font));

  int minx, maxx, miny, maxy, adv;
  TTF_GetGlyphMetrics(con->font, 'm', &minx, &maxx, &miny, &maxy, &adv);
  SDL_Log("MINX: %d | MAXX: %d | MINY: %d | MAXY: %d | ADV: %d\n", minx, maxx, miny, maxy, adv);

  TTF_GetStringSize(con->font, "m", 1, &maxx, &maxy);
  SDL_Log("W: %d, H: %d\n", maxx, maxy);
}

void console_write(struct console *con, const char *text, size_t len) {
  write(con->confd, text, len);
}

void console_update(struct console *con) {
  char buf[128];
  struct pollfd fds = {0};
  fds.fd = con->confd;
  fds.events = POLL_IN | POLL_ERR;
  if (poll(&fds, 1, 0) > 0) {
    if (fds.revents & POLL_IN) {
      size_t len = read(con->confd, buf, 128);
      if (len > 0) {
        console_process(con, buf, len);
      }
    }
  }
}

void console_draw(struct console *con) {
  int h, y = 0;
  TTF_GetTextSize(con->text, NULL, &h);
  if (h > con->window_height)
    y = con->window_height - h;
  TTF_DrawRendererText(con->text, 0, y);
}

/*------ Helper Functions ------*/

void console_process(struct console *con, const char *text, int len) {
  *con->SceneHasChanged = true;
  const char cur = (char)219;
  //TTF_SetTextString(con->text, text, len);
  TTF_AppendTextString(con->text, text, len);
  //TTF_AppendTextString(con->text, &cur, 1);
  const char *it = text;
  size_t plen = len;
  Uint32 ch; 
  int startesc = 0;
  while ((ch = SDL_StepUTF8(&it, &plen)) != '\0') {
    if (ch == '\e')
      startesc = 1;

    if (startesc) {
      if (startesc < 6) {
        ++startesc;
        SDL_Log("%x", ch);
      } else {
        startesc = 0;
        SDL_Log("\n");
      }
    }
  }
}
