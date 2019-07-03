// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "Surface.hpp"

#include <core/SkSurface.h>
#include <gpu/GrBackendSurface.h>
#include <gpu/GrContext.h>
#include <gpu/gl/GrGLTypes.h>
#include <src/gpu/gl/GrGLDefines.h>

#include "SkiaException.hpp"
#include "opengl/OpenGLHeaders.hpp"

using namespace wre_skia;

namespace {
sk_sp<SkSurface> create_surface(int width, int height, GLuint texture_name,
                                sk_sp<GrContext> skia_context) {
  log_debug("creating surface with texture %u", texture_name);
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
  glBindTexture(GL_TEXTURE_2D, 0);

  return surface;
}
}  // namespace

Surface::Surface(int width, int height, sk_sp<GrContext> context)
    : _backing_texture({width, height, GL_RGBA}),
      _surface(create_surface(width, height, _backing_texture._name, context)),
      _context(context) {}
