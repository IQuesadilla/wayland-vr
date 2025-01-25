#include "win2d.h"

struct win2d {
  SDL_Camera *cam;
  SDL_GPUDevice *dev;
  SDL_GPUTexture *tex;
  SDL_GPUTextureTransferInfo tti;
  SDL_GPUTextureRegion treg;
  int w, h, size;
  bool first;

  struct Transform t;
};

bool NewTex(struct win2d *cam, SDL_Surface *frame, bool delete) {
  if (cam->w != frame->w || cam->h != frame->h) {
    SDL_Log("Creating new texture for camera (%dx%d)", frame->w, frame->h);

    SDL_GPUTextureCreateInfo tci = {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
      .width = frame->w,
      .height = frame->h,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
    };
    SDL_GPUTexture *newtex = SDL_CreateGPUTexture(cam->dev, &tci);
    if (newtex == NULL)
      return false;

    int newsize = SDL_CalculateGPUTextureFormatSize(tci.format, tci.width, tci.height, tci.layer_count_or_depth);
    SDL_GPUTransferBufferCreateInfo tbci = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = newsize,
    };
    SDL_GPUTextureTransferInfo newtti = {
      .offset = 0,
      .pixels_per_row = frame->w,
      .rows_per_layer = frame->h,
      .transfer_buffer = SDL_CreateGPUTransferBuffer(cam->dev, &tbci)
    };
    if (newtti.transfer_buffer == NULL) {
      SDL_ReleaseGPUTexture(cam->dev, newtex);
      return false;
    }

    SDL_Log("Done %lu", sizeof(*cam));

    if (delete) {
      SDL_ReleaseGPUTexture(cam->dev, cam->tex);
      SDL_ReleaseGPUTransferBuffer(cam->dev, cam->tti.transfer_buffer);
    }

    cam->tex = newtex;
    cam->tti = newtti;
    cam->size = newsize;
    cam->w = frame->w;
    cam->h = frame->h;
    cam->treg = (SDL_GPUTextureRegion){
      .texture = cam->tex,
      .layer = 0,
      .w = frame->w,
      .h = frame->h,
      .d = 1
    };
  }

  void *pixels = SDL_MapGPUTransferBuffer(cam->dev, cam->tti.transfer_buffer, false);
  if (pixels == NULL)
    return false;
  SDL_memcpy(pixels, frame->pixels, cam->size);
  //SDL_memset(pixels, 0xFF, cam->size);
  SDL_UnmapGPUTransferBuffer(cam->dev, cam->tti.transfer_buffer);

  return true;
}

struct win2d *WayVR_Cam2D_Init(SDL_CameraID camid, SDL_GPUDevice *dev) {
  SDL_Camera *cam = SDL_OpenCamera(camid, NULL);

  struct win2d *obj = SDL_malloc(sizeof(struct win2d));
  obj->cam = cam;
  obj->dev = dev;
  obj->first = true;
  SDL_memset(&obj->t, 0, sizeof(obj->t));

  SDL_Surface *noinput = SDL_CreateSurface(64, 64, SDL_PIXELFORMAT_ARGB8888);
  SDL_Log("Uploading noinput");
  if (!NewTex(obj, noinput, false)) {
    SDL_Log("Failed creating new texture (%s)", SDL_GetError());
    SDL_free(obj);
    return NULL;
  }
  SDL_Log("done");
  SDL_DestroySurface(noinput);
  return obj;
}

void WayVR_Cam2D_Deinit(struct win2d *cam) {
  SDL_ReleaseGPUTexture(cam->dev, cam->tex);
  SDL_ReleaseGPUTransferBuffer(cam->dev, cam->tti.transfer_buffer);
  SDL_CloseCamera(cam->cam);
  SDL_free(cam);
}

bool WayVR_Cam2D_AcquireFrame(struct win2d *cam) {
  Uint64 ts;
  bool wasfirst = cam->first;
  cam->first = false;
  SDL_Surface *frame1 = SDL_AcquireCameraFrame(cam->cam, &ts);
  if (frame1 == NULL)
    return wasfirst;

  SDL_Surface *frame = SDL_ConvertSurface(frame1, SDL_PIXELFORMAT_ARGB8888);
  SDL_ReleaseCameraFrame(cam->cam, frame1);
  if (frame == NULL)
    return wasfirst;

  if (!NewTex(cam, frame, true))
    return wasfirst;

  //SDL_Log("Copying frame, Size: %d %d", cam->size, cam->valid_tex);
  //SDL_Log("Done");
  SDL_DestroySurface(frame);

  return true;
}

SDL_GPUTexture *WayVR_Cam2D_UploadFrame(struct win2d *cam, SDL_GPUCopyPass *CopyPass) {
  //SDL_Log("Uploading frame");
  SDL_UploadToGPUTexture(CopyPass, &cam->tti, &cam->treg, false);
  return cam->tex;
}

struct Transform *WayVR_Cam2D_GetTransform(struct win2d *win) {
  return &win->t;
}

void WayVR_Cam2D_CalculateModelMat(struct win2d *win, mat4 model) {
  glm_mat4_identity(model);
  versor quat;
  float h = win->h;
  float w = win->w;
  float x = glm_min(w / h, 1.f);// * win->t.Scale[1];
  float y = glm_min(h / w, 1.f);// * win->t.Scale[0];
  glm_translate(model, win->t.Translation);
  glm_euler_yxz_quat(win->t.Rotation, quat);
  glm_quat_rotate(model, quat, model);
  glm_scale(model, (vec3){x, y, 1.f});
}
