#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include "wayvr.h"

SDL_GPUShader* LoadShader(
  SDL_GPUDevice *dev,
  SDL_GPUShaderStage stage, //SDL_GPU_SHADERSTAGE_*
  const char *path,
  Uint32 num_samplers,
  Uint32 num_uniform_buffers,
  Uint32 num_storage_buffers,
  Uint32 num_storage_textures
) {
  SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(dev);
  if (!(backendFormats & SDL_GPU_SHADERFORMAT_SPIRV))
    return NULL;
  
  size_t len;
  void *code = SDL_LoadFile(path, &len);
  if (code == NULL)
    return NULL;

  SDL_GPUShaderCreateInfo shaderInfo = {
    .code = code,
    .code_size = len,
    .entrypoint = "main",
    .format = SDL_GPU_SHADERFORMAT_SPIRV,
    .stage = stage,
    .num_samplers = num_samplers,
    .num_uniform_buffers = num_uniform_buffers,
    .num_storage_buffers = num_storage_buffers,
    .num_storage_textures = num_storage_textures,
    .props = 0
  };
  SDL_GPUShader *shader = SDL_CreateGPUShader(dev, &shaderInfo);
  if (shader == NULL)
    return NULL;

  SDL_free(code);
  return shader;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  struct WayVR_Appstate *app = (struct WayVR_Appstate*)SDL_malloc(sizeof(struct WayVR_Appstate));
  *appstate = app;
  bool s;
  s = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_CAMERA | SDL_INIT_GAMEPAD);
  if (!s) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize SDL (%s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_Log("Asset Dir: %s", WAYVR_ASSET_DIR);

  int camcount;
  SDL_GetCameras(&camcount);
  SDL_Log("Camera Count: %d", camcount);
  /*if (camcount < 1) {
    SDL_Log("Not Enought Cameras! Quitting...");
    return SDL_APP_FAILURE;
  }*/

  int gpadcount;
  SDL_GetGamepads(&gpadcount);
  SDL_Log("Gamepad count: %d", gpadcount);
  /*if (gpadcount < 1) {
    SDL_Log("Not enough Gamepads! Quitting...");
    return SDL_APP_FAILURE;
  }*/
  SDL_SetGamepadEventsEnabled(true);

  app->dev = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
  if (app->dev == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create GPU device (%s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  //struct win2d *cwin = InitWin2D(IDs[0], dev);
  //if (cwin == NULL) {
    //SDL_LogCritical(SDL_LOG_CATEGORY_INPUT, "Failed to initialize win2d (%s)", SDL_GetError());
    //return SDL_APP_FAILURE;
  //}

  //SDL_GPUShader *vert = LoadShader(dev, SDL_GPU_SHADERSTAGE_VERTEX, WAYVR_ASSET_DIR "/assets/IndexedTriangle.vert.spv", 0, 0, 0, 0);
  SDL_GPUShader *vert = LoadShader(app->dev, SDL_GPU_SHADERSTAGE_VERTEX, WAYVR_ASSET_DIR "/assets/sample.vert.spv", 0, 1, 0, 0);
  //SDL_GPUShader *vert = LoadShader(dev, SDL_GPU_SHADERSTAGE_VERTEX, WAYVR_ASSET_DIR "/assets/RawTriangle.vert.spv", 0, 0, 0, 0);
  if (vert == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader (%s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_GPUShader *frag = LoadShader(app->dev, SDL_GPU_SHADERSTAGE_FRAGMENT, WAYVR_ASSET_DIR "/assets/sample.frag.spv", 1, 0, 0, 0);
  if (frag == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load fragment shader (%s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  /*float Vertices[] = {
    -1.f, -1.f, 0.f,
     1.f, -1.f, 0.f,
     0.f,  1.0, 0.f
  };
  float Colors[] = {
    1.f, 0.f, 0.f, 1.f,
    0.f, 1.f, 0.f, 1.f,
    0.f, 0.f, 1.f, 1.f
  };*/

  //SDL_GPUGraphicsPipeline *pipeline[count];
  SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {
    .vertex_input_state = (SDL_GPUVertexInputState){
      .num_vertex_buffers = 1,
      .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
        .slot = 0,
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        .instance_step_rate = 0,
        .pitch = sizeof(SDL_FPoint) * 2
      }},
      .num_vertex_attributes = 2,
      .vertex_attributes = (SDL_GPUVertexAttribute[]){{
        .buffer_slot = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
        .location = 0,
        .offset = 0
      },
      {
        .buffer_slot = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
        .location = 1,
        .offset = sizeof(SDL_FPoint)
      }}
    },
    .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
    .vertex_shader = vert,
    .fragment_shader = frag,
    .rasterizer_state = {
      .fill_mode = SDL_GPU_FILLMODE_FILL,
    },
    .multisample_state = {
      .sample_count = SDL_GPU_SAMPLECOUNT_1,
    },
    .depth_stencil_state = {
      .enable_depth_test = true,
      .enable_depth_write = true,
      .compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
      .enable_stencil_test = false,
    }
  };

  int count = 0;
  app->DisplayList = InitDisplays(app->dev, pipelineInfo);
  UpdateDisplays(app->DisplayList, &count);

  struct SDL_GPUBufferCreateInfo buffInfo = {
    .size = sizeof(Vertices) + sizeof(UVs),
    .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
    .props = 0
  };
  SDL_GPUBuffer *buff = SDL_CreateGPUBuffer(app->dev, &buffInfo);
  if (buff == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create GPU buffer (%s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_GPUBufferRegion reg = {
    .buffer = buff,
    .size = sizeof(Vertices) + sizeof(UVs),
    .offset = 0
  };

  SDL_GPUTransferBufferCreateInfo tbci = {
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = sizeof(Vertices) + sizeof(UVs),
    .props = 0
  };
  SDL_GPUTransferBuffer *tbuff = SDL_CreateGPUTransferBuffer(app->dev, &tbci);
  if (tbuff == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create GPU transfer buffer (%s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_GPUTransferBufferLocation loc = {
    .transfer_buffer = tbuff,
    .offset = 0
  };

  SDL_Surface *bmp1 = SDL_LoadBMP(WAYVR_ASSET_DIR "/assets/uvtemplate.bmp");
  if (bmp1 == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "Failed to load bmp (%s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_Surface *bmp = SDL_ConvertSurface(bmp1, SDL_PIXELFORMAT_ARGB8888);
  SDL_DestroySurface(bmp1);
  if (bmp == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "Failed to convert bmp (%s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  int imageSizeInBytes = bmp->w * bmp->h * 4;
  SDL_Log("BMP: %dx%d * 4 = %d", bmp->w, bmp->h, imageSizeInBytes);

  SDL_GPUTextureCreateInfo tci = {
    .type = SDL_GPU_TEXTURETYPE_2D,
    .format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
    .width = bmp->w,
    .height = bmp->h,
    .layer_count_or_depth = 1,
    .num_levels = 1,
    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
  };
  SDL_GPUTexture *WallpaperTexture = SDL_CreateGPUTexture(app->dev, &tci);
  if (WallpaperTexture == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create texture (%s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_GPUTextureRegion treg = {
    .texture = WallpaperTexture,
    .layer = 0,
    .w = bmp->w,
    .h = bmp->h,
    .d = 1,
  };

  SDL_GPUSamplerCreateInfo sci = {
    .min_filter = SDL_GPU_FILTER_LINEAR,
    .mag_filter = SDL_GPU_FILTER_LINEAR,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
  };
  SDL_GPUSampler *Sampler = SDL_CreateGPUSampler(app->dev, &sci);
  if (Sampler == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create sampler (%s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_GPUTransferBufferCreateInfo textbuffci = {
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = imageSizeInBytes,
  };
  SDL_GPUTransferBuffer* textbuff = SDL_CreateGPUTransferBuffer(app->dev, &textbuffci);
  if (textbuff == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create GPU texture transfer buffer (%s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_GPUTextureTransferInfo textinfo = {
    .transfer_buffer = textbuff,
    .offset = 0,
    .pixels_per_row = bmp->w,
    .rows_per_layer = bmp->h
  };

  SDL_FPoint *mem = SDL_MapGPUTransferBuffer(app->dev, tbuff, false);
  if (mem == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to map GPU transfer buffer (%s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  for (int k = 0; k < 6; ++k) {
    int m = k * 2;
    mem[m  ] = Vertices[k];
    mem[m+1] = UVs[k];
  }
  SDL_UnmapGPUTransferBuffer(app->dev, tbuff);

  void *pixels = SDL_MapGPUTransferBuffer(app->dev, textbuff, false);
  if (pixels == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to map GPU texture transfer buffer (%s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  //SDL_memset(pixels, 0xFF, imageSizeInBytes);
  //SDL_memcpy(pixels, bmp->pixels, imageSizeInBytes);
  SDL_memmove(pixels, bmp->pixels, imageSizeInBytes);
  SDL_UnmapGPUTransferBuffer(app->dev, textbuff);

  SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(app->dev);
  if (cmdbuf == NULL)
    return SDL_APP_FAILURE;

  SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(cmdbuf);
  SDL_UploadToGPUBuffer(copyPass, &loc, &reg, false);
  SDL_UploadToGPUTexture(copyPass, &textinfo, &treg, false);
  SDL_EndGPUCopyPass(copyPass);
  //SDL_GenerateMipmapsForGPUTexture(cmdbuf, WallpaperTexture);
  s = SDL_SubmitGPUCommandBuffer(cmdbuf);
  if (!s) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to submit GPU cmd buffer (%s)", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_ReleaseGPUTransferBuffer(app->dev, tbuff);
  SDL_ReleaseGPUTransferBuffer(app->dev, textbuff);

  app->vertBinding = (SDL_GPUBufferBinding){
    .offset = 0,
    .buffer = buff
  };

  app->WallpaperSampler = (SDL_GPUTextureSamplerBinding){
    .sampler = Sampler,
    .texture = WallpaperTexture
  };

  mat4 WallpaperMVP;
  glm_mat4_identity(WallpaperMVP);

  app->WindowSampler = (SDL_GPUTextureSamplerBinding){
    .sampler = Sampler,
    .texture = NULL
  };

  SDL_ReleaseGPUShader(app->dev, vert);
  SDL_ReleaseGPUShader(app->dev, frag);

  SDL_Log("Init completed, starting loop");

  app->curgpad = NULL;
  SDL_memset(&app->Movement, 0, sizeof(struct Transform));
  app->WinList = WayVR_WinList_Init();
  app->cwin = NULL;
  app->SelectWin = false;
  app->Config = DefaultConfiguraion;

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  struct WayVR_Appstate *app = (struct WayVR_Appstate*)appstate;
  SDL_DestroyGPUDevice(app->dev);
  return;
}
