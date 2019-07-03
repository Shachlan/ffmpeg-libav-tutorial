#pragma once

#include <SkRefCnt.h>

class SkSurface;
class GrContext;

namespace wre_skia {
struct SurfaceInfo {
  SurfaceInfo(uint32_t texture_name, sk_sp<SkSurface> surface, sk_sp<GrContext> context)
      : backing_texture_name(texture_name), surface(surface), context(context) {}
  SurfaceInfo() = default;

  uint32_t backing_texture_name;
  sk_sp<SkSurface> surface;
  sk_sp<GrContext> context;
};
}  // namespace wre_skia
