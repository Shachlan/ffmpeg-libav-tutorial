#include "SurfacePool.hpp"

#include <GrContext.h>
#include <OpenGL/gl3.h>
#include <SkSurface.h>
#include <gpu/GrBackendSurface.h>
#include <src/gpu/gl/GrGLDefines.h>

#include "SkiaException.hpp"
#include "SurfaceInfo.hpp"
#include "opengl/TextureAllocator.hpp"

using namespace WRESkiaRendering;

static sk_sp<SkSurface> create_surface(int width, int height, GLuint texture_name,
                                       sk_sp<GrContext> skia_context) {
  log_info("creating surface with texture %u", texture_name);
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

SurfacePool::SurfacePool(sk_sp<GrContext> context,
                         std::shared_ptr<WREOpenGL::TextureAllocator> texture_allocator,
                         int surface_width, int surface_height)
    : context(context),
      texture_allocator(texture_allocator),
      surface_width(surface_width),
      surface_height(surface_height) {
}

SurfacePool::~SurfacePool() {
  surfaces.clear();
}

unique_ptr<SurfaceInfo> SurfacePool::get_surface_info() {
  unique_ptr<SurfaceInfo> surface_info = nullptr;
  if (surfaces.empty()) {
    auto texture = WREOpenGL::TextureAllocator::get_texture_info(surface_width, surface_height,
                                                                 GL_RGBA, texture_allocator);
    surface_info = std::make_unique<SurfaceInfo>(
        texture, create_surface(surface_width, surface_height, texture->name, context), context);
  } else {
    auto begin_iter = surfaces.begin();
    surface_info = std::move(*begin_iter);
    surfaces.erase(begin_iter);
  }

  auto canvas = surface_info->surface->getCanvas();
  canvas->clear(SkColorSetARGB(255, 0, 0, 0));

  return surface_info;
}

void SurfacePool::release_surface(SurfaceInfo &surface_info) {
  surfaces.emplace_back(std::make_unique<SurfaceInfo>(std::move(surface_info)));
}
