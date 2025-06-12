//const std = @import("std");
//const builtin = @import("builtin");
const os = @import("std").os;
const SDL = @cImport({
    @cInclude("SDL3/SDL.h");
    @cInclude("SDL3_ttf/SDL_ttf.h");
});

fn EnumerateDirectoryCallback(nstate: ?*anyopaque, dirname: [*c]const u8, fname: [*c]const u8) callconv(.C) SDL.SDL_EnumerationResult {
    if (fname[0] == '.')
        return SDL.SDL_ENUM_CONTINUE;

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

        const newbtn = &state.Children[state.ChildCount];
        _ = SDL.SDL_asprintf(&newbtn.Path, "%s%s", dirname, fname);

        var PathInfo: SDL.SDL_PathInfo = undefined;
        if (!SDL.SDL_GetPathInfo(newbtn.Path, &PathInfo)) {
            SDL.SDL_Log("Failed to get path info [%s]: %s\n", newbtn.Path, SDL.SDL_GetError());
        } else {
            //newbtn.Name.len = @intCast(SDL.SDL_asprintf(@ptrCast(&newbtn.Name.ptr), "%s", fname));
            newbtn.Name = SDL.TTF_CreateText(state.state.tengine, state.state.font, fname, 0) orelse return SDL.SDL_ENUM_CONTINUE;

            newbtn.TypeChar = switch (PathInfo.type) {
                SDL.SDL_PATHTYPE_NONE => 'N',
                SDL.SDL_PATHTYPE_FILE => 'F',
                SDL.SDL_PATHTYPE_DIRECTORY => 'D',
                SDL.SDL_PATHTYPE_OTHER => 'O',
                else => '?',
            };

            //const shift: isize = @intCast(SDL.SDL_utf8strlen(state.Name));
            //const width: isize = @intCast(SDL.SDL_utf8strlen(newbtn.Name));
            newbtn.IsMouseOver = false;
            newbtn.pos.x = 1; //state.pos.x + (shift * 8) + 4;
            newbtn.pos.y = 1; //state.pos.y + @as(isize, @intCast(12 * state.ChildCount));
            newbtn.pos.w = 1; //(width * 8) + 2;
            newbtn.pos.h = state.pos.h;
            newbtn.Selected = false;
            newbtn.Children = &[0]Button{};
            newbtn.ChildCount = 0;
            newbtn.state = state.state;

            state.ChildCount += 1;
        }
    }
    return SDL.SDL_ENUM_CONTINUE;
}

const Rectangle = struct {
    x: isize,
    y: isize,
    z: isize,
    w: isize,
    h: isize,
};

const Button = struct {
    pos: Rectangle,
    IsMouseOver: bool,
    Name: *SDL.TTF_Text,
    Path: [*c]u8,
    Children: []Button,
    ChildCount: usize,
    TypeChar: c_char,
    Selected: bool,
    state: *AppState,

    fn MoveTo(self: *Button, x: isize, y: isize, screenw: usize) void {
        self.pos.x = x - @divTrunc(self.pos.w, 2);
        self.pos.y = y - @divTrunc(self.pos.h, 2);
        for (0..self.ChildCount) |k| {
            self.Children[k].pos.w = @intCast(@divTrunc(screenw, self.ChildCount + 1));
            self.Children[k].MoveTo(@intCast(@divTrunc(screenw, self.ChildCount + 1) * (k + 1)), self.pos.y + (self.pos.h * 2) + 2, screenw);
        }
    }

    fn UpdateIsMouseOver(self: *Button, x: isize, y: isize) void {
        self.IsMouseOver = (x > self.pos.x and x < self.pos.x + self.pos.w and y > self.pos.y and y < self.pos.y + self.pos.h);
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
            SDL.TTF_DestroyText(self.Name);
            SDL.SDL_free(self.Path);
        }
        self.Selected = false;
    }

    fn UpdateChildren(self: *Button) void {
        switch (self.TypeChar) {
            'D' => {
                if (self.Children.len == 0) {
                    _ = SDL.SDL_EnumerateDirectory(self.Path, EnumerateDirectoryCallback, self);
                    self.Selected = self.Children.len > 0;
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

    fn TriggerClicked(self: *Button) bool {
        if (self.IsMouseOver) {
            self.UpdateChildren();
            return true;
            //SDL.SDL_Log("Button Clicked! %d %d\n", self.Children.len, self.ChildCount);
        }

        var del: isize = -1;
        for (0..self.ChildCount) |k| {
            if (del > -1) {
                self.Children[k].Dealloc(true);
            } else {
                if (self.Children[k].TriggerClicked()) {
                    del = @intCast(k);
                }
            }
        }
        if (del > 0) {
            for (0..@intCast(del)) |k| {
                self.Children[k].Dealloc(true);
            }
        }
        return false;
    }

    fn Render(self: *Button) !void {
        var w: c_int = 0;
        var h: c_int = 0;
        _ = SDL.TTF_GetTextSize(self.Name, &w, &h);

        const DrawRect: SDL.SDL_FRect = .{
            .x = @as(f32, @floatFromInt(self.pos.x)),
            .y = @as(f32, @floatFromInt(self.pos.y)),
            .w = @as(f32, @floatFromInt(self.pos.w)),
            .h = @as(f32, @floatFromInt(self.pos.h)),
        };
        const blue: u8 = if (self.IsMouseOver) 255 else 30;
        const green: u8 = if (self.Selected) 150 else 30;

        _ = SDL.SDL_SetRenderDrawColor(self.state.rend, 255, 255, 255, 255);

        _ = SDL.SDL_SetRenderDrawColor(self.state.rend, 30, green, blue, 255);
        _ = SDL.SDL_RenderFillRect(self.state.rend, &DrawRect);

        const halfwidth: f32 = DrawRect.w / 2; //@floatFromInt(@divTrunc(self.pos.w, 2));
        var halftwidth: f32 = @as(f32, @floatFromInt(w)) / 2;

        var Name: *SDL.TTF_Text = self.Name;
        var cutoff: bool = false;
        if (halftwidth > halfwidth) {
            var TName: []u8 = undefined;
            TName.len = @intCast(SDL.SDL_asprintf(@ptrCast(&TName.ptr), "%.4s...", self.Name.text));
            defer SDL.SDL_free(TName.ptr);

            Name = SDL.TTF_CreateText(self.state.tengine, self.state.font, TName.ptr, TName.len) orelse return error.GeneralFailure;
            _ = SDL.TTF_GetTextSize(Name, &w, &h);
            halftwidth = @as(f32, @floatFromInt(w)) / 2;

            cutoff = true;
        }

        //_ = SDL.SDL_RenderDebugText(state.rend, 1 + DrawRect.x + halfwidth - halftwidth, DrawRect.y + 1, Name);
        _ = SDL.TTF_DrawRendererText(Name, 1 + DrawRect.x + halfwidth - halftwidth, DrawRect.y + 1);

        if (cutoff) {
            SDL.TTF_DestroyText(Name);
        }

        var k = self.ChildCount;
        while (k > 0) {
            k -= 1;
            try self.Children[k].Render();
        }
    }
};

const AppState = struct {
    win: *SDL.SDL_Window,
    rend: *SDL.SDL_Renderer,
    tengine: *SDL.TTF_TextEngine,
    font: *SDL.TTF_Font,
    btn: Button,
    StartTime: SDL.SDL_Time,
    Ticks: u64,

    fn ResizeWin(self: *AppState) void {
        var winw: c_int = 0;
        var winh: c_int = 0;
        if (SDL.SDL_GetWindowSize(self.win, &winw, &winh)) {
            self.btn.MoveTo(@divFloor(winw, 2), 10, @intCast(winw));
        }
    }
};

pub fn AppInit(state: *AppState, argv: [][*:0]u8) !void {
    _ = argv;
    if (!SDL.SDL_Init(SDL.SDL_INIT_VIDEO)) {
        SDL.SDL_Log("Couldn't initialize SDL: %s", SDL.SDL_GetError());
        return error.GeneralFailure;
    }

    if (!SDL.TTF_Init()) {
        SDL.SDL_Log("Failed to initialize TTF: %s", SDL.SDL_GetError());
        return error.GeneralFailure;
    }

    var fsize: f64 = 16;
    const env: *SDL.SDL_Environment = SDL.SDL_GetEnvironment() orelse return error.GeneralFailure;
    const wvr_fontsize: [*c]const u8 = SDL.SDL_GetEnvironmentVariable(env, "WVR_FONTSIZE");
    if (wvr_fontsize) |ptr| {
        const temp_fsize: f64 = SDL.SDL_strtod(ptr, null);
        if (temp_fsize != 0) {
            fsize = temp_fsize;
        }
    }
    SDL.SDL_Log("FontSize: %f\n", fsize);

    const win: *SDL.SDL_Window = SDL.SDL_CreateWindow(",,", 1024, 720, SDL.SDL_WINDOW_RESIZABLE | SDL.SDL_WINDOW_TRANSPARENT) orelse return error.GeneralFailure;
    const rend: *SDL.SDL_Renderer = SDL.SDL_CreateRenderer(win, null) orelse return error.GeneralFailure;
    if (!SDL.SDL_SetRenderVSync(rend, 1)) {
        SDL.SDL_Log("Failed to set render vsync (%s)\n", SDL.SDL_GetError());
    }
    const tengine: *SDL.TTF_TextEngine = SDL.TTF_CreateRendererTextEngine(rend) orelse return error.GeneralFailure;
    const font: *SDL.TTF_Font = SDL.TTF_OpenFont("./font.ttf", @floatCast(fsize)) orelse return error.GeneralFailure;
    const Name: *SDL.TTF_Text = SDL.TTF_CreateText(tengine, font, "Home", 0) orelse return error.GeneralFailure;

    var w: c_int = 0;
    var h: c_int = 0;
    _ = SDL.TTF_GetTextSize(Name, &w, &h);

    var ctime: SDL.SDL_Time = 0;
    _ = SDL.SDL_GetCurrentTime(&ctime);

    //var ret: *AppState = @alignCast(@ptrCast(SDL.SDL_malloc(@sizeOf(AppState))));
    state.* = .{
        .win = win,
        .rend = rend,
        .tengine = tengine,
        .font = font,
        .StartTime = ctime,
        .Ticks = 0,
        .btn = .{
            .pos = .{
                .x = 1,
                .y = 10,
                .z = 0,
                .w = w + 2,
                .h = h + 2,
            },
            .IsMouseOver = false,
            .TypeChar = 'D',
            .Children = &[0]Button{},
            .ChildCount = 0,
            .Name = Name,
            .Path = undefined,
            .Selected = false,
            .state = state,
        },
    };
    //_ = SDL.SDL_asprintf(&ret.text, "");

    //ret.btn.Name.len = @intCast(SDL.SDL_asprintf(@ptrCast(&ret.btn.Name.ptr), "Home"));
    _ = SDL.SDL_asprintf(&state.btn.Path, "/."); // "%s", SDL.SDL_GetUserFolder(SDL.SDL_FOLDER_HOME));
    //SDL.SDL_Log("ALL: %s\n", ret.text);

    state.ResizeWin();
    return;
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
            _ = state.btn.TriggerClicked();
            state.ResizeWin();
        },
        SDL.SDL_EVENT_WINDOW_RESIZED => {
            state.ResizeWin();
        },
        else => {},
    }
    return true;
}
pub fn AppIterate(state: *AppState) !bool {
    _ = SDL.SDL_SetRenderDrawColorFloat(state.rend, 0.0, 0.0, 0.0, SDL.SDL_ALPHA_TRANSPARENT_FLOAT);
    _ = SDL.SDL_RenderClear(state.rend);

    try state.btn.Render();

    _ = SDL.SDL_RenderPresent(state.rend);

    state.Ticks += 1;

    return true;
}
pub fn AppQuit(state: *AppState) void {
    var ctime: SDL.SDL_Time = 0;
    _ = SDL.SDL_GetCurrentTime(&ctime);

    const frames: f32 = @floatFromInt(state.Ticks);
    const seconds: f32 = @as(f32, @floatFromInt(ctime - state.StartTime)) / 1_000_000_000;
    const fps: f32 = frames / seconds;
    SDL.SDL_Log("Avg FPS: %f | Frames: %f | Seconds: %f\n", fps, frames, seconds);

    SDL.TTF_DestroyText(state.btn.Name);
    SDL.TTF_DestroyRendererTextEngine(state.tengine);
    SDL.TTF_CloseFont(state.font);
    SDL.SDL_DestroyRenderer(state.rend);
    SDL.SDL_DestroyWindow(state.win);
    SDL.TTF_Quit();
    SDL.SDL_Quit();
}

pub fn main() !void {
    var state: AppState = undefined;
    try AppInit(&state, os.argv);
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
