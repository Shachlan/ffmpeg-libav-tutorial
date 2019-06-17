#pragma once

#include <SkRefCnt.h>

#include "opengl/TextureInfo.hpp"

class SkSurface;
class GrContext;

namespace WRESkiaRendering {
struct SurfaceInfo {
  SurfaceInfo(unique_ptr<WREOpenGL::TextureInfo> &backing_texture, sk_sp<SkSurface> surface,
              sk_sp<GrContext> context)
      : backing_texture(std::move(backing_texture)), surface(surface), context(context) {
  }
  SurfaceInfo() = default;

  unique_ptr<WREOpenGL::TextureInfo> backing_texture;
  sk_sp<SkSurface> surface;
  sk_sp<GrContext> context;
};
}  // namespace WRESkiaRendering
