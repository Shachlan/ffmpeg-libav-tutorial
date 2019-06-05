#include "SkiaSurface.hpp"

#include <GrContext.h>
#include <OpenGL/gl3.h>
#include <SkRefCnt.h>
#include <SkSurface.h>
#include <gpu/GrBackendSurface.h>
#include <src/gpu/gl/GrGLDefines.h>

#include "SkiaException.hpp"

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

class SkiaSurface::Impl {
public:
  Impl(int width, int height, uint32_t texture_name, sk_sp<GrContext> skia_context)
      : texture_name(texture_name),
        skia_surface(create_surface2(width, height, texture_name, skia_context)) {
  }

  const GLuint texture_name;

private:
  const sk_sp<SkSurface> skia_surface;
};

SkiaSurface::SkiaSurface(int width, int height, uint32_t texture_name,
                         sk_sp<GrContext> skia_context)
    : implementation(
          std::make_unique<SkiaSurface::Impl>(width, height, texture_name, skia_context)) {
}
