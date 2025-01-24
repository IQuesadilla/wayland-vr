#include "console.h"

#include <SDL3_ttf/SDL_ttf.h>
#include <unistd.h>
#include <pty.h>
#include <signal.h>
#include <poll.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

int console_process(struct console *con, const char *text, int len);

/*------ Console Object Definition ------*/

struct dirtyrect {
  struct dirtyrect *next;
  SDL_Rect rect;
};

struct console {
  TTF_TextEngine *tengine;
  TTF_Font *font;
  TTF_Text *text;
  int window_height;
  //SDL_Process *proc;
  int confd;
  pid_t cpid;
  int curx, cury;
  int doRedraw;
  struct dirtyrect *dirtyrects;
};

/*------ Console Object Functions ------*/

bool console_init(struct console **con, SDL_Renderer* rend) {
  struct console *newcon = (struct console*)SDL_malloc(sizeof(struct console));

  newcon->tengine = TTF_CreateRendererTextEngine(rend);
  if (newcon->tengine == NULL) return false;

  bool s = console_setfontsize(newcon, 12);
  if (newcon->font == NULL || !s) return false;

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
  if (kill(con->cpid, SIGHUP) == 0)
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

bool console_setfontsize(struct console *con, int sz) {
  TTF_Font *newfont = TTF_OpenFont("font.ttf", 16);
  if (newfont == NULL) return false;

  if (!TTF_FontIsFixedWidth(newfont)) {
    SDL_LogError(SDL_LOG_CATEGORY_RENDER, "FONT IS NOT MONOSPACE!\n");
    TTF_CloseFont(newfont);
    con->font = NULL;
    return false;
  }
  con->font = newfont;

  SDL_LogInfo(SDL_LOG_CATEGORY_INPUT, "Font is %s\n", TTF_GetFontFamilyName(con->font));

  int minx, maxx, miny, maxy, adv;
  TTF_GetGlyphMetrics(con->font, 'm', &minx, &maxx, &miny, &maxy, &adv);
  SDL_Log("MINX: %d | MAXX: %d | MINY: %d | MAXY: %d | ADV: %d\n", minx, maxx, miny, maxy, adv);

  TTF_GetStringSize(con->font, "m", 1, &maxx, &maxy);
  SDL_Log("W: %d, H: %d\n", maxx, maxy);
  return true;
}

void console_write(struct console *con, const char *text, size_t len) {
  write(con->confd, text, len);
}

int console_update(struct console *con) {
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
  return con->doRedraw;
}

void console_draw(struct console *con) {
  // NOTE: In this case, the dirty rectangles should refer to locations, not pixels
  int h, y = 0;
  TTF_GetTextSize(con->text, NULL, &h);
  if (h > con->window_height)
    y = con->window_height - h;
  TTF_DrawRendererText(con->text, 0, y);
  con->doRedraw = 0;
}

/*------ Helper Functions ------*/

//void console_cleardirtyrects(struct console *con)

void console_addredraw(struct console *con, SDL_Rect *region) {
  if (region == NULL) {
    con->doRedraw = 2; // Do a clear and redraw
    struct dirtyrect *it = con->dirtyrects;
    while (it != NULL) {
      struct dirtyrect *next = it->next;
      SDL_free(it);
      it = next;
    }
  } else if (con->doRedraw > 1) { // Don't bother with dirty rects if already 2
    con->doRedraw = 1; // Only redraw dirty rects
    struct dirtyrect *newitem = SDL_malloc(sizeof(struct dirtyrect));
    newitem->next = NULL;
    newitem->rect = *region;
    if (con->dirtyrects == NULL) {
      con->dirtyrects = newitem;
    } else {
      struct dirtyrect *last = con->dirtyrects;
      while (last->next != NULL) last = last->next;
      last->next = newitem;
    }
  }
}

int console_process(struct console *con, const char *text, int len) {
  //*con->SceneHasChanged = true;
  //const char cur = (char)219;
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
  return 0;
}
