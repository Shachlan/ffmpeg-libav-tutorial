// Cop /right (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#pragma once

#include <core/SkRefCnt.h>
#include <core/SkSurface.h>
#include <gpu/GrContext.h>

#include "opengl/Texture.hpp"

namespace WRESkiaRendering {
/// Information needed to use a Skia surface.
struct Surface {
  Surface(unique_ptr<WREOpenGL::Texture> &&backing_texture, sk_sp<SkSurface> surface,
          sk_sp<GrContext> context)
      : surface(surface), context(context), backing_texture(std::move(backing_texture)) {}
  Surface() = default;
  Surface &operator=(const Surface &other) = delete;
  Surface(const Surface &other) = delete;
  Surface &operator=(Surface &&other) = default;
  Surface(Surface &&other) = default;

  /// Skia Surface.
  sk_sp<SkSurface> surface;

  /// Skia context backing the surface.
  sk_sp<GrContext> context;

  /// Name of the OpenGL texture backing the surface.
  unique_ptr<WREOpenGL::Texture> backing_texture;
};
}  // namespace WRESkiaRendering
