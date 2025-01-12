#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdbool.h>
#include <SDL3/SDL.h>

struct console;

bool console_init(struct console **con, SDL_Renderer* rend, bool *SceneHasChanged);
void console_deinit(struct console *con);
void console_resize(struct console *con, int w, int h);
void console_setfontsize(struct console *con, int sz);
void console_write(struct console *con, const char *text, size_t len);
void console_update(struct console *con);
void console_draw(struct console *con);

#endif
