//const std = @import("std");
const SDL = @cImport({
    @cInclude("SDL3/SDL.h");
});

fn EnumerateDirectoryCallback(nstate: ?*anyopaque, dirname: [*c]const u8, fname: [*c]const u8) callconv(.C) SDL.SDL_EnumerationResult {
    if (nstate) |astate| {
        const state: *Button = @alignCast(@ptrCast(astate));
        if (state.ChildCount == state.Children.len) {
            if (state.ChildCount == 0) {
                state.Children.len = 10;
                state.Children.ptr = @alignCast(@ptrCast(SDL.SDL_malloc(state.Children.len * @sizeOf(Button))));
            } else {
                state.Children.len += 10;
                state.Children.ptr = @alignCast(@ptrCast(SDL.SDL_realloc(state.Children.ptr, state.Children.len * @sizeOf(Button))));
            }
        }
        //const newbtn: *Button = SDL.SDL_malloc();

        const newbtn = &state.Children[state.ChildCount];
        _ = SDL.SDL_asprintf(&newbtn.Path, "%s%s", dirname, fname);

        var PathInfo: SDL.SDL_PathInfo = undefined;
        if (!SDL.SDL_GetPathInfo(newbtn.Path, &PathInfo)) {
            SDL.SDL_Log("Failed to get path info [%s]: %s\n", newbtn.Path, SDL.SDL_GetError());
        } else {
            _ = SDL.SDL_asprintf(&newbtn.Name, "%s", fname);
            newbtn.TypeChar = switch (PathInfo.type) {
                SDL.SDL_PATHTYPE_NONE => 'N',
                SDL.SDL_PATHTYPE_FILE => 'F',
                SDL.SDL_PATHTYPE_DIRECTORY => 'D',
                SDL.SDL_PATHTYPE_OTHER => 'O',
                else => '?',
            };

            const shift: isize = @intCast(SDL.SDL_utf8strlen(state.Name));
            const width: isize = @intCast(SDL.SDL_utf8strlen(newbtn.Name));
            newbtn.IsMouseOver = false;
            newbtn.x = state.x + (shift * 8) + 4;
            newbtn.y = state.y + @as(isize, @intCast(12 * state.ChildCount));
            newbtn.w = (width * 8) + 2;
            newbtn.h = state.h;
            //newbtn.Children.len = 0;
            newbtn.Children = &[0]Button{};
            newbtn.ChildCount = 0;

            state.ChildCount += 1;
        }
    }
    return SDL.SDL_ENUM_CONTINUE;
}

const Button = struct {
    x: isize,
    y: isize,
    w: isize,
    h: isize,
    IsMouseOver: bool,
    Name: [*c]u8,
    Path: [*c]u8,
    Children: []Button,
    ChildCount: usize,
    TypeChar: c_char,

    fn UpdateIsMouseOver(self: *Button, x: isize, y: isize) void {
        self.IsMouseOver = (x > self.x and x < self.x + self.w and y > self.y and y < self.y + self.h);
        for (0..self.ChildCount) |k| {
            self.Children[k].UpdateIsMouseOver(x, y);
        }
    }

    fn Dealloc(self: *Button, first: bool) void {
        if (self.ChildCount > 0) {
            for (0..self.ChildCount) |k| {
                self.Children[k].Dealloc(false);
            }
            SDL.SDL_free(self.Children.ptr);
            self.ChildCount = 0;
            self.Children.len = 0;
        }
        if (!first) {
            SDL.SDL_free(self.Name);
            SDL.SDL_free(self.Path);
        }
    }

    fn UpdateChildren(self: *Button) void {
        switch (self.TypeChar) {
            'D' => {
                if (self.Children.len == 0) {
                    _ = SDL.SDL_EnumerateDirectory(self.Path, EnumerateDirectoryCallback, self);
                } else {
                    self.Dealloc(true);
                }
            },
            'F' => {
                var url: [*c]u8 = undefined;
                _ = SDL.SDL_asprintf(&url, "file://%s", self.Path);
                defer SDL.SDL_free(url);

                if (!SDL.SDL_OpenURL(url)) {
                    SDL.SDL_Log("Failed to open url %s - %s\n", url, SDL.SDL_GetError());
                }
            },
            else => {},
        }
    }

    fn TriggerClicked(self: *Button) void {
        if (self.IsMouseOver) {
            self.UpdateChildren();
            SDL.SDL_Log("Button Clicked! %d %d\n", self.Children.len, self.ChildCount);
        }

        for (0..self.ChildCount) |k| {
            self.Children[k].TriggerClicked();
        }
    }

    fn Render(self: *Button, rend: *SDL.SDL_Renderer) !void {
        const DrawRect: SDL.SDL_FRect = .{
            .x = @as(f32, @floatFromInt(self.x)),
            .y = @as(f32, @floatFromInt(self.y)),
            .w = @as(f32, @floatFromInt(self.w)),
            .h = @as(f32, @floatFromInt(self.h)),
        };
        const blue: u8 = if (self.IsMouseOver) 255 else 30;
        _ = SDL.SDL_SetRenderDrawColor(rend, 30, 30, blue, 255);
        _ = SDL.SDL_RenderFillRect(rend, &DrawRect);

        _ = SDL.SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
        _ = SDL.SDL_RenderDebugText(rend, DrawRect.x + 1, DrawRect.y + 1, self.Name);

        var k = self.ChildCount;
        while (k > 0) {
            k -= 1;
            try self.Children[k].Render(rend);
        }
    }
};

const AppState = struct {
    win: *SDL.SDL_Window,
    rend: *SDL.SDL_Renderer,
    btn: Button,
};

pub fn AppInit() !AppState {
    if (!SDL.SDL_Init(SDL.SDL_INIT_VIDEO)) {
        SDL.SDL_Log("Couldn't initialize SDL: %s", SDL.SDL_GetError());
        return error.GeneralFailure;
    }

    const win: *SDL.SDL_Window = SDL.SDL_CreateWindow(",,", 640, 480, SDL.SDL_WINDOW_RESIZABLE | SDL.SDL_WINDOW_TRANSPARENT) orelse return error.GeneralFailure;
    const rend: *SDL.SDL_Renderer = SDL.SDL_CreateRenderer(win, null) orelse return error.GeneralFailure;

    var ret: AppState = .{
        .win = win,
        .rend = rend,
        .btn = .{
            .x = 1,
            .y = 1,
            .w = (4 * 8) + 2,
            .h = 10,
            .IsMouseOver = false,
            .TypeChar = 'D',
            .Children = &[0]Button{},
            .ChildCount = 0,
            .Name = undefined,
            .Path = undefined,
        },
    };
    //_ = SDL.SDL_asprintf(&ret.text, "");

    _ = SDL.SDL_asprintf(&ret.btn.Name, "Home");
    _ = SDL.SDL_asprintf(&ret.btn.Path, "%s", SDL.SDL_GetUserFolder(SDL.SDL_FOLDER_HOME));
    //SDL.SDL_Log("ALL: %s\n", ret.text);

    return ret;
}

pub fn AppEvent(state: *AppState, e: SDL.SDL_Event) !bool {
    switch (e.type) {
        SDL.SDL_EVENT_QUIT => {
            return false;
        },
        SDL.SDL_EVENT_MOUSE_MOTION => {
            state.btn.UpdateIsMouseOver(@intFromFloat(e.motion.x), @intFromFloat(e.motion.y));
        },
        SDL.SDL_EVENT_MOUSE_BUTTON_DOWN => {
            state.btn.TriggerClicked();
        },
        else => {},
    }
    return true;
}
pub fn AppIterate(state: *AppState) !bool {
    _ = SDL.SDL_SetRenderDrawColorFloat(state.rend, 0.0, 0.0, 0.0, SDL.SDL_ALPHA_TRANSPARENT_FLOAT);
    _ = SDL.SDL_RenderClear(state.rend);

    try state.btn.Render(state.rend);

    //_ = SDL.SDL_SetRenderDrawColor(state.rend, 255, 255, 255, 255);
    //_ = SDL.SDL_RenderDebugText(state.rend, 0.0, 0.0, state.text);

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
