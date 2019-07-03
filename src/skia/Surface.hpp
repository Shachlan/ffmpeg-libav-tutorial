// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#pragma once

#include <core/SkRefCnt.h>

#include "opengl/Texture.hpp"

class SkSurface;
class GrContext;

namespace wre_skia {
/// RAII wrapper for Skia surface, backed by an OpenGL texture.
struct Surface {
  Surface(int width, int height, sk_sp<GrContext> context);
  Surface &operator=(const Surface &other) = delete;
  Surface(const Surface &other) = delete;
  Surface &operator=(Surface &&other) = default;
  Surface(Surface &&other) = default;

  /// Texture backing the surface.
  const wre_opengl::Texture _backing_texture;

  /// Skia Surface.
  const sk_sp<SkSurface> _surface;

  /// Skia context backing the surface.
  const sk_sp<GrContext> _context;
};
}  // namespace wre_skia
