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
  //SDL_IOStream *master, *slave;
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

  //newcon->master = SDL_IOFromFile("/dev/ptmx", "r+b");
  //SDL_PropertiesID mprop = SDL_GetIOProperties(newcon->master);
  //int fd = SDL_GetNumberProperty(mprop, SDL_PROP_IOSTREAM_FILE_DESCRIPTOR_NUMBER, -1);
  //SDL_Log("%d %s %llu", fd, ttyname(fd), newcon->master);
  //grantpt(fd);
  //unlockpt(fd);
  //char *slave_path = ptsname();
  //SDL_IOFromFile(slave_path, "r+");

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

  //SDL_PropertiesID props = SDL_CreateProperties();
  //SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, args);
  //SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDIN_NUMBER, SDL_PROCESS_STDIO_REDIRECT);
  //SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_REDIRECT);
  //SDL_SetPointerProperty(props, SDL_PROP_PROCESS_STDIN_POINTER, newcon->in);
  //SDL_SetPointerProperty(props, SDL_PROP_PROCESS_STDOUT_POINTER, newcon->out);
  //SDL_SetBooleanProperty(props, SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN, true);
  //newcon->proc = SDL_CreateProcessWithProperties(props);
  //newcon->in = SDL_GetProcessInput(newcon->proc);
  //newcon->out = SDL_GetProcessOutput(newcon->proc);

  *con = newcon;
  return true;
}

void console_deinit(struct console *con) {
  //SDL_KillProcess(con->proc, false);
  //SDL_CloseIO(con->master);
  //SDL_CloseIO(con->slave);
  //SDL_DestroyProcess(con->proc);
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

void console_append(struct console *con, const char *text, size_t len) {
  //TTF_AppendTextString(con->text, text, len);
  //SDL_WriteIO(con->master, text, len);
  write(con->confd, text, len);
}

void console_update(struct console *con) {
  char buf[128];
  //size_t len = SDL_ReadIO(con->master, buf, 128);
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
