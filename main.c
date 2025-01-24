#include <SDL3/SDL.h>
#include "displays.h"
#include "win2d.h"
#include <cglm/cglm.h>

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

int main(int argc, char *argv[]) {
  bool s;
  s = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_CAMERA | SDL_INIT_GAMEPAD);
  if (!s) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize SDL (%s)", SDL_GetError());
    return 1;
  }

  SDL_Log("Asset Dir: %s", WAYVR_ASSET_DIR);

  int camcount;
  SDL_GetCameras(&camcount);
  SDL_Log("Camera Count: %d", camcount);
  if (camcount < 1) {
    SDL_Log("Not Enought Cameras! Quitting...");
    return 0;
  }

  int gpadcount;
  SDL_GetGamepads(&gpadcount);
  SDL_Log("Gamepad count: %d", gpadcount);
  /*if (gpadcount < 1) {
    SDL_Log("Not enough Gamepads! Quitting...");
    return 0;
  }*/
  SDL_SetGamepadEventsEnabled(true);

  SDL_GPUDevice *dev = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
  if (dev == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create GPU device (%s)", SDL_GetError());
    return 1;
  }

  //struct win2d *cwin = InitWin2D(IDs[0], dev);
  //if (cwin == NULL) {
    //SDL_LogCritical(SDL_LOG_CATEGORY_INPUT, "Failed to initialize win2d (%s)", SDL_GetError());
    //return 1;
  //}

  //SDL_GPUShader *vert = LoadShader(dev, SDL_GPU_SHADERSTAGE_VERTEX, WAYVR_ASSET_DIR "/assets/IndexedTriangle.vert.spv", 0, 0, 0, 0);
  SDL_GPUShader *vert = LoadShader(dev, SDL_GPU_SHADERSTAGE_VERTEX, WAYVR_ASSET_DIR "/assets/sample.vert.spv", 0, 1, 0, 0);
  //SDL_GPUShader *vert = LoadShader(dev, SDL_GPU_SHADERSTAGE_VERTEX, WAYVR_ASSET_DIR "/assets/RawTriangle.vert.spv", 0, 0, 0, 0);
  if (vert == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load vertex shader (%s)", SDL_GetError());
    return 1;
  }

  SDL_GPUShader *frag = LoadShader(dev, SDL_GPU_SHADERSTAGE_FRAGMENT, WAYVR_ASSET_DIR "/assets/sample.frag.spv", 1, 0, 0, 0);
  if (frag == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to load fragment shader (%s)", SDL_GetError());
    return 1;
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
  const float I = 1.f;
  SDL_FPoint Vertices[] = {
    {-I,  I},
    { I,  I},
    {-I, -I},
    {-I, -I},
    { I, -I},
    { I,  I},
  };
  SDL_FPoint UVs[] = {
    {0.f, 0.f},
    {1.f, 0.f},
    {0.f, 1.f},
    {0.f, 1.f},
    {1.f, 1.f},
    {1.f, 0.f},
  };
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
    }
  };

  int count = 0;
  struct displays *disp = InitDisplays(dev, pipelineInfo);
  UpdateDisplays(disp, &count);

  struct SDL_GPUBufferCreateInfo buffInfo = {
    .size = sizeof(Vertices) + sizeof(UVs),
    .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
    .props = 0
  };
  SDL_GPUBuffer *buff = SDL_CreateGPUBuffer(dev, &buffInfo);
  if (buff == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create GPU buffer (%s)", SDL_GetError());
    return 1;
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
  SDL_GPUTransferBuffer *tbuff = SDL_CreateGPUTransferBuffer(dev, &tbci);
  if (tbuff == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create GPU transfer buffer (%s)", SDL_GetError());
    return 1;
  }
  SDL_GPUTransferBufferLocation loc = {
    .transfer_buffer = tbuff,
    .offset = 0
  };

  SDL_Surface *bmp1 = SDL_LoadBMP(WAYVR_ASSET_DIR "/assets/uvtemplate.bmp");
  if (bmp1 == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "Failed to load bmp (%s)", SDL_GetError());
    return 1;
  }
  SDL_Surface *bmp = SDL_ConvertSurface(bmp1, SDL_PIXELFORMAT_ARGB8888);
  SDL_DestroySurface(bmp1);
  if (bmp == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "Failed to convert bmp (%s)", SDL_GetError());
    return 1;
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
  SDL_GPUTexture *WallpaperTexture = SDL_CreateGPUTexture(dev, &tci);
  if (WallpaperTexture == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create texture (%s)", SDL_GetError());
    return 1;
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
  SDL_GPUSampler *Sampler = SDL_CreateGPUSampler(dev, &sci);
  if (Sampler == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create sampler (%s)", SDL_GetError());
    return 1;
  }

  SDL_GPUTransferBufferCreateInfo textbuffci = {
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = imageSizeInBytes,
  };
  SDL_GPUTransferBuffer* textbuff = SDL_CreateGPUTransferBuffer(dev, &textbuffci);
  if (textbuff == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to create GPU texture transfer buffer (%s)", SDL_GetError());
    return 1;
  }
  SDL_GPUTextureTransferInfo textinfo = {
    .transfer_buffer = textbuff,
    .offset = 0,
    .pixels_per_row = bmp->w,
    .rows_per_layer = bmp->h
  };

  SDL_FPoint *mem = SDL_MapGPUTransferBuffer(dev, tbuff, false);
  if (mem == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to map GPU transfer buffer (%s)", SDL_GetError());
    return 1;
  }
  for (int k = 0; k < 6; ++k) {
    int m = k * 2;
    mem[m  ] = Vertices[k];
    mem[m+1] = UVs[k];
  }
  SDL_UnmapGPUTransferBuffer(dev, tbuff);

  void *pixels = SDL_MapGPUTransferBuffer(dev, textbuff, false);
  if (pixels == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to map GPU texture transfer buffer (%s)", SDL_GetError());
    return 1;
  }
  //SDL_memset(pixels, 0xFF, imageSizeInBytes);
  //SDL_memcpy(pixels, bmp->pixels, imageSizeInBytes);
  SDL_memmove(pixels, bmp->pixels, imageSizeInBytes);
  SDL_UnmapGPUTransferBuffer(dev, textbuff);

  SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(dev);
  if (cmdbuf == NULL)
    return 1;

  SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(cmdbuf);
  SDL_UploadToGPUBuffer(copyPass, &loc, &reg, false);
  SDL_UploadToGPUTexture(copyPass, &textinfo, &treg, false);
  SDL_EndGPUCopyPass(copyPass);
  //SDL_GenerateMipmapsForGPUTexture(cmdbuf, WallpaperTexture);
  s = SDL_SubmitGPUCommandBuffer(cmdbuf);
  if (!s) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to submit GPU cmd buffer (%s)", SDL_GetError());
    return 1;
  }

  SDL_ReleaseGPUTransferBuffer(dev, tbuff);
  SDL_ReleaseGPUTransferBuffer(dev, textbuff);

  SDL_GPUBufferBinding vertBinding = {
    .offset = 0,
    .buffer = buff
  };

  SDL_GPUTextureSamplerBinding WallpaperSampler = {
    .sampler = Sampler,
    .texture = WallpaperTexture
  };

  mat4 WallpaperMVP;
  glm_mat4_identity(WallpaperMVP);
  //vec4 Pos = {1.f, 1.f, 0.f, 1.f};
  //glm_mat4_mulv(WallpaperMVP, Pos, Pos);
  //for (int i = 0; i < 4; ++i)
      //SDL_Log("%d = %f", i, Pos[i]);

  SDL_GPUTextureSamplerBinding WindowSampler = {
    .sampler = Sampler,
    .texture = NULL
  };

  SDL_ReleaseGPUShader(dev, vert);
  SDL_ReleaseGPUShader(dev, frag);

  SDL_Log("Init completed, starting loop");

  bool loop = true;
  SDL_Gamepad *curgpad = NULL;
  struct win2d *win = NULL;
  while (loop) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_EVENT_QUIT:
          loop = false;
          break;
        case SDL_EVENT_KEY_DOWN:
          switch (e.key.key) {
            case SDLK_ESCAPE:
              SDL_Log("Found Esc!");
              loop = false;
              break;
          }
          break;
        case SDL_EVENT_CAMERA_DEVICE_ADDED:
          if (win == NULL) {
            SDL_Log("Found new camera!");
            win = InitWin2D(e.cdevice.which, dev);
          } else {
            SDL_Log("One camera is already connected");
          }
          break;
        case SDL_EVENT_CAMERA_DEVICE_REMOVED:
          SDL_Log("Removed camera!");
          DeinitWin2D(win);
          win = NULL;
          break;
        case SDL_EVENT_GAMEPAD_ADDED:
          if (curgpad == NULL) {
            SDL_Log("Found new gamepad!");
            curgpad = SDL_OpenGamepad(e.gdevice.which);
          } else {
            SDL_Log("One gamepad already connected");
          }
          break;
        case SDL_EVENT_GAMEPAD_REMOVED:
          SDL_Log("Removed gamepad!");
          SDL_CloseGamepad(curgpad);
          curgpad = NULL;
          break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
          switch (e.gbutton.button) {
            case SDL_GAMEPAD_BUTTON_START:
              SDL_Log("Found Start!");
              loop = false;
              break;
            default:
              SDL_Log("Gamepad button down: %s", SDL_GetGamepadStringForButton(e.gbutton.button));
              break;
          }
          break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
          //SDL_Log("Gamepad axis motion: %d %s", e.gaxis.value, SDL_GetGamepadStringForAxis(e.gaxis.axis));
          if (win != NULL) {
            struct Transform *t = GetTransform(win);
            switch (e.gaxis.axis) {
              case SDL_GAMEPAD_AXIS_LEFTX:
                t->Translation[0] = ((float)e.gaxis.value) / 32768.f;
                break;
              case SDL_GAMEPAD_AXIS_LEFTY:
                t->Translation[1] = ((float)e.gaxis.value) / -32768.f;
                break;
              case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
                t->Translation[2] = ((float)e.gaxis.value) / -32768.f;
                break;
              case SDL_GAMEPAD_AXIS_RIGHTX:
                t->Rotation[1] = ((float)e.gaxis.value) / 32768.f;
                //versor newrot;
                //glm_rotate(t->Rotation, ((float)e.gaxis.value) / 32768.f, 0.f, 1.f, 0.f);
                //glm_quat_mul(t->Rotation, newrot, t->Rotation);
              break;
              case SDL_GAMEPAD_AXIS_RIGHTY:
                t->Rotation[0] = ((float)e.gaxis.value) / 32768.f;
                //glm_quat(t->Rotation, ((float)e.gaxis.value) / -32768.f, 1.f, 0.f, 0.f);
                break;
              case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
                //t->Rotation[0] = ((float)e.gaxis.value) / -32768.f;
                break;
            }
          }
          break;
      }
    }

    bool newframe = AcquireFrame(win);

    SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(dev);
    if (cmdbuf == NULL)
      return 1;

    if (newframe) {
      SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cmdbuf);
      WindowSampler.texture = UploadFrame(win, cp);
      SDL_EndGPUCopyPass(cp);
    }

    for (struct displaynode *it = GetDisplayList(disp); it != NULL; it = it->next) {
      SDL_PushGPUVertexUniformData(cmdbuf, 0, WallpaperMVP, sizeof(mat4));
      mat4 FrameViewMat, FramePerspMat;
      CalculateVPMatrix(it, FrameViewMat, FramePerspMat);
      SDL_GPURenderPass *renderPass = BeginRenderPass(it, cmdbuf);
      SDL_BindGPUVertexBuffers(renderPass, 0, &vertBinding, 1);
      SDL_BindGPUFragmentSamplers(renderPass, 0, &WallpaperSampler, 1);
      SDL_DrawGPUPrimitives(renderPass, 6, 1, 0, 0);
      if (true) {
        mat4 FrameModelMat, FrameMVP;
        CalculateModelMat(win, FrameModelMat);
        glm_mat4_mul(FrameViewMat, FrameModelMat, FrameMVP);
        glm_mat4_mul(FramePerspMat, FrameMVP, FrameMVP);
        SDL_PushGPUVertexUniformData(cmdbuf, 0, FrameMVP, sizeof(mat4));
        SDL_BindGPUFragmentSamplers(renderPass, 0, &WindowSampler, 1);
        SDL_DrawGPUPrimitives(renderPass, 6, 1, 0, 0);
      }
      SDL_EndGPURenderPass(renderPass);
      /*SDL_BlitGPUTexture(cmdbuf, &(SDL_GPUBlitInfo){
        .source.texture = WallpaperTexture,
        .source.w = bmp->w,
        .source.h = bmp->h,
        .destination.texture = swapchainTexture,
        .destination.w = bmp->w,
        .destination.h = bmp->h,
        .load_op = SDL_GPU_LOADOP_DONT_CARE,
        .filter = SDL_GPU_FILTER_LINEAR
      });*/
    }
    s = SDL_SubmitGPUCommandBuffer(cmdbuf);
    if (!s) {
      SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to submit loop GPU cmd buffer (%s)", SDL_GetError());
      return 1;
    }
  }
  SDL_DestroyGPUDevice(dev);
}
