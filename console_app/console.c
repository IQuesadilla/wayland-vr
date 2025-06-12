#include "console.h"

#include <unistd.h>
#include <pty.h>
#include <signal.h>
#include <poll.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <SDL3/SDL.h>

struct console_updatelist *console_process(struct console *con, const char *text, int len);

/*------ Console Object Definition ------*/

struct dirtyrect {
  struct dirtyrect *next;
  SDL_Rect rect;
};

struct console {
  //int window_height;
  //SDL_Process *proc;
  int confd;
  pid_t cpid;
  SDL_Point cur;
  SDL_Point tsz;
  struct dirtyrect *dirtyrects;
};

/*------ Console Object Functions ------*/

bool console_init(struct console **con, SDL_Renderer* rend) {
  struct console *newcon = (struct console*)SDL_malloc(sizeof(struct console));

  //int w, h;
  //SDL_Window *win = SDL_GetRenderWindow(rend);
  //SDL_GetWindowSize(win, &w, &h);
  //console_resize(newcon, w, h);

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
  newcon->cur.x = 0;
  newcon->cur.y = 0;

  *con = newcon;
  return true;
}

void console_deinit(struct console *con) {
  if (kill(con->cpid, SIGHUP) == 0)
    waitpid(con->cpid, NULL, 0);
}

void console_resize(struct console *con, SDL_Point sz) {
  //con->window_height = 0;

  con->tsz = sz;
  struct winsize wsz = {0};
  wsz.ws_col = sz.x;
  wsz.ws_row = sz.y;

  SDL_LogDebug(SDL_LOG_CATEGORY_RENDER, "Resize: w: %d, h: %d\n", wsz.ws_col, wsz.ws_row);
  ioctl(con->confd, TIOCSWINSZ, &sz);
}

void console_write(struct console *con, const char *text, size_t len) {
  write(con->confd, text, len);
}

struct console_updatelist *console_update(struct console *con) {
  char buf[128];
  struct pollfd fds = {0};
  fds.fd = con->confd;
  fds.events = POLL_IN | POLL_ERR;
  if (poll(&fds, 1, 0) > 0) {
    if (fds.revents & POLL_IN) {
      size_t len = read(con->confd, buf, 128);
      if (len > 0) {
        return console_process(con, buf, len);
      }
    }
  }
  return NULL;
}

/*void console_draw(struct console *con) {

}*/

/*------ Helper Functions ------*/

//void console_cleardirtyrects(struct console *con)

/*void console_addredraw(struct console *con, SDL_Rect *region) {
  if (region == NULL) {
    //con->doRedraw = 2; // Do a clear and redraw
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
}*/

struct console_updatelist *console_process(struct console *con, const char *text, int len) {
  //*con->SceneHasChanged = true;
  //const char cur = (char)219;
  struct console_updatelist *ret = NULL;
  struct console_updatelist *lit = NULL;
  const char *it = text;
  size_t plen = len;
  Uint32 ch; 
  int newlinesup = 0;
  char *cit = NULL;
  while ((ch = SDL_StepUTF8(&it, &plen)) != 0) {
    bool donewitem = false;
    if (ch == '\e') {
      ch = 0;//'~';
      //SDL_Log("%x", *it); //NOTE: Prints [ (0x5b)
      switch (it[0]) {
        case '[':
          switch (it[1]) {
            case '?':
              switch (it[2]) {
                case '2':
                  switch (it[3]) {
                    case '5': // ~[?25h
                      it += 5;
                      plen -= 5;
                      break;
                    case '0': // ~[?2004h
                      it += 7;
                      plen -= 7;
                      break;
                  }
                  break;
              }
              break;
            case 'K': // ~[K -> ~[0K
              it += 2;
              plen -= 2;
              break;
            case 'm': // ~[m -> ~[0m
              it += 2;
              plen -= 2;
              break;
          }
          break;
        case '(': // ~(B
          switch (it[1]) {
            case 'B':
              it += 2;
              plen -= 2;
              break;
          }
          break;
      }
    }
    {
      if (con->cur.x >= con->tsz.x || ch == '\n') {
        donewitem = true;
        con->cur.x = 0;
        con->cur.y += 1;
        if (con->cur.y >= con->tsz.y) {
          newlinesup = con->cur.y - con->tsz.y + 1;
          con->cur.y = con->tsz.y - 1;
          //SDL_Log("New lines up: %d %d", newlinesup, con->cur.y);
        }
        ch = 0;
      }
      if (ch == '\r') {
        donewitem = true;
        ch = 0;
        con->cur.x = 0;
        //con->cur.y += 1;
      }

      if (donewitem || ret == NULL) {
        struct console_updatelist *newitem = SDL_malloc(sizeof(struct console_updatelist));
        newitem->Text = SDL_malloc(256);
        newitem->Length = 0;
        newitem->LinesUp = newlinesup;
        newitem->Location = con->cur;
        newitem->next = NULL;
        if (lit == NULL) {
          ret = newitem;
        } else {
          //lit->Text[lit->Length] = 0;
          lit->next = newitem;
        }
        cit = newitem->Text;
        lit = newitem;
      }
      if (ch != 0) {
        /*if (cit - lit->Text > 250) {
          SDL_Log("reallocing");
          lit->Text = SDL_realloc(lit->Text, lit->Length * 2);
          cit = lit->Text + lit->Length;
        }*/
        char *oldcit = cit;
        cit = SDL_UCS4ToUTF8(ch, cit);
        lit->Length += (cit - oldcit);
        con->cur.x += 1;
      }
    }
  }
  //if (lit != NULL)
    //lit->Text[lit->Length+1] = 0;
  //SDL_Log("Dont with hh");
  return ret;
}
