//const std = @import("std");
const SDL = @cImport({
    @cInclude("SDL3/SDL.h");
});

const AppState = struct {
    win: *SDL.SDL_Window,
    rend: *SDL.SDL_Renderer,
};

pub fn AppInit() !AppState {
    if (!SDL.SDL_Init(SDL.SDL_INIT_VIDEO)) {
        SDL.SDL_Log("Couldn't initialize SDL: %s", SDL.SDL_GetError());
        return error.GeneralFailure;
    }

    const win: *SDL.SDL_Window = SDL.SDL_CreateWindow(",,", 640, 480, SDL.SDL_WINDOW_RESIZABLE) orelse return error.GeneralFailure;
    const rend: *SDL.SDL_Renderer = SDL.SDL_CreateRenderer(win, null) orelse return error.GeneralFailure;

    const ret: AppState = .{ .win = win, .rend = rend };
    return ret;
}

pub fn AppEvent(state: *AppState, e: SDL.SDL_Event) !bool {
    _ = state;
    switch (e.type) {
        SDL.SDL_EVENT_QUIT => {
            return false;
        },
        else => {},
    }
    return true;
}
pub fn AppIterate(state: *AppState) !bool {
    const now: f64 = @as(f64, @floatFromInt(SDL.SDL_GetTicks())) / 1000.0; // convert from milliseconds to seconds.
    //choose the color for the frame we will draw. The sine wave trick makes it fade between colors smoothly.
    const red: f32 = 0.5 + 0.5 * @as(f32, @floatCast(SDL.SDL_sin(now)));
    const green: f32 = 0.5 + 0.5 * @as(f32, @floatCast(SDL.SDL_sin(now + SDL.SDL_PI_D * 2 / 3)));
    const blue: f32 = 0.5 + 0.5 * @as(f32, @floatCast(SDL.SDL_sin(now + SDL.SDL_PI_D * 4 / 3)));
    _ = SDL.SDL_SetRenderDrawColorFloat(state.rend, red, green, blue, SDL.SDL_ALPHA_OPAQUE_FLOAT); //new color, full alpha.
    _ = SDL.SDL_RenderClear(state.rend);
    _ = SDL.SDL_RenderPresent(state.rend);

    return true;
}
pub fn AppQuit(state: *AppState) void {
    _ = state;
}

pub fn main() !void {
    var state: AppState = try AppInit();
    defer AppQuit(&state);

    var loop: bool = true;
    mainloop: while (loop) {
        var e: SDL.SDL_Event = .{ .type = 0 };
        while (SDL.SDL_PollEvent(&e)) {
            if (!try AppEvent(&state, e))
                break :mainloop;
        }
        loop = try AppIterate(&state);
    }
}
