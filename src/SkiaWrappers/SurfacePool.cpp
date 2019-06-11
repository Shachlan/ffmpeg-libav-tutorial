#include "SurfacePool.hpp"

#include <GrContext.h>
#include <OpenGL/gl3.h>
#include <SkSurface.h>
#include <gpu/GrBackendSurface.h>
#include <src/gpu/gl/GrGLDefines.h>

#include "SkiaException.hpp"
#include "opengl/TexturePool.hpp"

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
                         std::shared_ptr<WREOpenGL::TexturePool> texture_pool, int surface_width,
                         int surface_height)
    : context(context),
      texture_pool(texture_pool),
      surface_width(surface_width),
      surface_height(surface_height) {
}

SurfacePool::~SurfacePool() {
  // TODO - release all textures
}

SurfaceInfo SurfacePool::get_surface_info() {
  SurfaceInfo surface_info;
  if (available_surfaces.empty()) {
    auto texture_name = texture_pool->get_texture();
    surface_info =
        SurfaceInfo(texture_name,
                    create_surface(surface_width, surface_height, texture_name, context), context);
  } else {
    auto begin_iter = available_surfaces.begin();
    surface_info = *begin_iter;
    available_surfaces.erase(begin_iter);
  }

  used_surfaces.insert(used_surfaces.begin(), surface_info);
  auto canvas = surface_info.surface->getCanvas();
  canvas->clear(SkColorSetARGB(255, 0, 0, 0));

  return surface_info;
}

void SurfacePool::release_surface(SurfaceInfo surface_info) {
  for (auto iter = used_surfaces.begin(); iter != used_surfaces.end(); iter++) {
    if ((*iter).backing_texture_name == surface_info.backing_texture_name) {
      used_surfaces.erase(iter);
      break;
    }
  }
  available_surfaces.insert(available_surfaces.begin(), surface_info);
}
