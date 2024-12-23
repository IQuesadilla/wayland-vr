#include "console.h"

#include <SDL3_ttf/SDL_ttf.h>
#include <unistd.h>
#include <pty.h>
#include <signal.h>
#include <poll.h>
#include <sys/wait.h>

struct console {
  TTF_TextEngine *tengine;
  TTF_Font *font;
  TTF_Text *text;
  int window_height;
  SDL_Process *proc;
  int confd;
  pid_t cpid;
  bool *SceneHasChanged;
};

bool console_init(struct console **con, SDL_Renderer* rend, bool *SceneHasChanged) {
  struct console *newcon = (struct console*)SDL_malloc(sizeof(struct console));
  newcon->SceneHasChanged = SceneHasChanged;
  *newcon->SceneHasChanged = true;

  newcon->tengine = TTF_CreateRendererTextEngine(rend);
  if (newcon->tengine == NULL) return false;

  newcon->font = TTF_OpenFont("font.ttf", 16);
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
    return -1;
  } else if (cpid == 0) {
    execv(args[0], args);
    return 1;
  }
  newcon->cpid = cpid;

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
        TTF_AppendTextString(con->text, buf, len);
        *con->SceneHasChanged = true;
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
