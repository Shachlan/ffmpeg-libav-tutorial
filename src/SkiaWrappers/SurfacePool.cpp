// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "SurfacePool.hpp"

#include <gpu/GrBackendSurface.h>
#include <gpu/GrContext.h>
#include <gpu/gl/GrGlTypes.h>
#include <src/gpu/gl/GrGLDefines.h>

#include "SkiaException.hpp"
#include "Surface.hpp"
#include "opengl/OpenGLHeaders.hpp"

using namespace WRESkiaRendering;

static sk_sp<SkSurface> create_surface(int width, int height, GLuint texture_name,
                                       sk_sp<GrContext> skia_context) {
  log_info("creating surface with texture %u", texture_name);
  glBindTexture(GL_TEXTURE_2D, texture_name);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  GrGLTextureInfo texture_info = {GL_TEXTURE_2D, texture_name, GR_GL_RGBA8};
  GrBackendTexture texture(width, height, GrMipMapped::kNo, texture_info);
  auto surface =
      sk_sp(SkSurface::MakeFromBackendTexture(skia_context.get(), texture, kTopLeft_GrSurfaceOrigin,
                                              0, kRGBA_8888_SkColorType, nullptr, nullptr));
  if (!surface) {
    throw SkiaException("Could not create skia surface.");
  }

  return surface;
}

SurfacePool::SurfacePool(sk_sp<GrContext> context, int surface_width, int surface_height)
    : surface_width(surface_width), surface_height(surface_height), context(context) {
}

SurfacePool::~SurfacePool() {
  flush();
}

unique_ptr<Surface> SurfacePool::get_surface() {
  if (available_surfaces.empty()) {
    auto texture = WREOpenGL::Texture::make_texture(surface_width, surface_height, GL_RGBA);
    auto surface = create_surface(surface_width, surface_height, texture->name, context);
    return std::make_unique<Surface>(std::move(texture), surface, context);
  }

  auto begin_iter = available_surfaces.begin();
  auto surface = std::move(*begin_iter);
  available_surfaces.erase(begin_iter);
  return surface;
}

void SurfacePool::release_surface(Surface &surface) {
  available_surfaces.insert(available_surfaces.begin(),
                            std::make_unique<Surface>(std ::move(surface.backing_texture),
                                                      surface.surface, surface.context));
}

void SurfacePool::flush() {
  available_surfaces.clear();
}
