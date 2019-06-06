#include "SkiaSurfacePool.hpp"

#include <GrContext.h>
#include <OpenGL/gl3.h>
#include <SkSurface.h>
#include <gpu/GrBackendSurface.h>
#include <src/gpu/gl/GrGLDefines.h>

#include "SkiaException.hpp"
#include "opengl/TexturePool.hpp"

using namespace WRESkiaRendering;

static sk_sp<SkSurface> create_surface2(int width, int height, GLuint texture_name,
                                        sk_sp<GrContext> skia_context) {
  glBindTexture(GL_TEXTURE_2D, texture_name);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  GrGLTextureInfo texture_info = {
      .fID = texture_name, .fTarget = GL_TEXTURE_2D, .fFormat = GR_GL_RGBA8};
  GrBackendTexture texture(width, height, GrMipMapped::kNo, texture_info);
  auto surface =
      sk_sp(SkSurface::MakeFromBackendTexture(skia_context.get(), texture, kTopLeft_GrSurfaceOrigin,
                                              0, kRGBA_8888_SkColorType, nullptr, nullptr));
  if (!surface) {
    throw SkiaException("Could not create skia surface.");
  }

  return surface;
}

SkiaSurfacePool::SkiaSurfacePool(std::shared_ptr<TexturePool> texture_pool, int surface_width,
                                 int surface_height)
    : texture_pool(texture_pool), surface_width(surface_width), surface_height(surface_height) {
}

sk_sp<SkSurface> SkiaSurfacePool::get_surface() {
  return nullptr;
}