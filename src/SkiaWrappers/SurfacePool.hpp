#include <SkRefCnt.h>
#include <vector>

#include "SkiaWrappers/SurfaceInfo.hpp"

namespace WREOpenGL {
class TextureAllocator;
}

namespace WRESkiaRendering {
class SurfacePool {
public:
  SurfacePool(sk_sp<GrContext> context,
              std::shared_ptr<WREOpenGL::TextureAllocator> texture_allocator, int surface_width,
              int surface_height);
  ~SurfacePool();
  unique_ptr<SurfaceInfo> get_surface_info();
  void release_surface(SurfaceInfo &surface_info);

private:
  const sk_sp<GrContext> context;
  const std::shared_ptr<WREOpenGL::TextureAllocator> texture_allocator;
  const int surface_width;
  const int surface_height;
  std::vector<unique_ptr<SurfaceInfo>> surfaces;
};
}  // namespace WRESkiaRendering
