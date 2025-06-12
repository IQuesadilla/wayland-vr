#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdbool.h>
#include <SDL3/SDL.h>

struct console;

struct console_updatelist {
  struct console_updatelist *next;
  SDL_Point Location;
  int Length;
  char *Text;
  int LinesUp;
};

bool console_init(struct console **con, SDL_Renderer* rend);
void console_deinit(struct console *con);
void console_resize(struct console *con, SDL_Point sz);
//bool console_setfontsize(struct console *con, int sz);
void console_write(struct console *con, const char *text, size_t len);
struct console_updatelist *console_update(struct console *con);
//void console_draw(struct console *con);

#endif
