#include "wayvr.h"

SDL_AppResult SDL_AppIterate(void *appstate) {
  struct WayVR_Appstate *app = (struct WayVR_Appstate*)appstate;
  {
    struct Transform *tp, *dtp = GetDisplayTransform(GetNextDisplay(app->DisplayList, NULL));
    versor quat;
    if (app->cwin != NULL) {
      tp = WayVR_GetTransform(app->cwin);
      glm_vec3_sub(tp->Rotation, app->Movement.Rotation, tp->Rotation);
      glm_euler_yxz_quat(dtp->Rotation, quat);
    } else {
      tp = dtp;
      glm_vec3_add(tp->Rotation, app->Movement.Rotation, tp->Rotation);
      glm_euler_yxz_quat((vec3){0.f, dtp->Rotation[1], 0.f}, quat);
    }
    vec3 trans;
    glm_quat_rotatev(quat, app->Movement.Translation, trans);
    glm_vec3_add(trans, tp->Translation, tp->Translation);
  }

  bool newframe = WayVR_AcquireFrame(app->win);

  SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(app->dev);
  if (cmdbuf == NULL)
    return 1;

  if (newframe) {
    SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cmdbuf);
    app->WindowSampler.texture = WayVR_UploadFrame(app->win, cp);
    SDL_EndGPUCopyPass(cp);
  }

  float CandidateDistance = INFINITY;
  for (struct display *it = GetNextDisplay(app->DisplayList, NULL); it != NULL; it = GetNextDisplay(app->DisplayList, it)) {
    SDL_PushGPUVertexUniformData(cmdbuf, 0, GLM_MAT4_IDENTITY, sizeof(mat4));
    mat4 FrameViewMat, FramePerspMat;
    CalculateVPMatrix(it, FrameViewMat, FramePerspMat);
    SDL_GPURenderPass *renderPass = BeginRenderPass(it, cmdbuf);
    SDL_BindGPUVertexBuffers(renderPass, 0, &app->vertBinding, 1);
    SDL_BindGPUFragmentSamplers(renderPass, 0, &app->WallpaperSampler, 1);
    SDL_DrawGPUPrimitives(renderPass, 6, 1, 0, 0);
    struct window *win_it = app->win;
    if (win_it->type != WAYVR_WINDOWTYPE_NONE) {
      mat4 FrameModelMat, FrameMVP;
      WayVR_CalculateModelMat(win_it, FrameModelMat);
      glm_mat4_mul(FrameViewMat, FrameModelMat, FrameMVP);
      glm_mat4_mul(FramePerspMat, FrameMVP, FrameMVP);
      SDL_PushGPUVertexUniformData(cmdbuf, 0, FrameMVP, sizeof(mat4));
      SDL_BindGPUFragmentSamplers(renderPass, 0, &app->WindowSampler, 1);
      SDL_DrawGPUPrimitives(renderPass, 6, 1, 0, 0);

      if (app->SelectWin) {
        vec3 TVerts[6];
        for (int k = 0; k < 6; ++k) {
          vec4 full = {Vertices[k].x, Vertices[k].y, 0.f, 1.f};
          glm_mat4_mulv(FrameModelMat, full, full);
          glm_vec3_divs(full, full[3], TVerts[k]);
        }
        versor rot;
        vec3 ray;
        struct Transform *t = GetDisplayTransform(it);
        glm_euler_yxz_quat(t->Rotation, rot);
        glm_quat_rotatev(rot, (vec3){0.f, 0.f, -1.f}, ray);
        float dis1, dis2;
        bool IsCasted1 = glm_ray_triangle(t->Translation, ray, TVerts[0], TVerts[1], TVerts[2], &dis1);
        bool IsCasted2 = glm_ray_triangle(t->Translation, ray, TVerts[3], TVerts[4], TVerts[5], &dis2);
        if (IsCasted1 && dis1 < CandidateDistance) {
          app->cwin = win_it;
          CandidateDistance = dis1;
        } else
        if (IsCasted2 && dis2 < CandidateDistance) {
          app->cwin = win_it;
          CandidateDistance = dis2;
        }
      }
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
  bool s = SDL_SubmitGPUCommandBuffer(cmdbuf);
  if (!s) {
    SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "Failed to submit loop GPU cmd buffer (%s)", SDL_GetError());
    return 1;
  }

  app->SelectWin = false;
  return SDL_APP_CONTINUE;
}
