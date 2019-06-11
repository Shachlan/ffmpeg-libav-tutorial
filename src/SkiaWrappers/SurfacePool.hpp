#include <SkRefCnt.h>
#include <vector>

#include "SkiaWrappers/SurfaceInfo.hpp"

namespace WREOpenGL {
class TexturePool;
}

namespace WRESkiaRendering {
class SurfacePool {
public:
  SurfacePool(sk_sp<GrContext> context, std::shared_ptr<WREOpenGL::TexturePool> texture_pool,
              int surface_width, int surface_height);
  ~SurfacePool();
  SurfaceInfo get_surface_info();
  void release_surface(SurfaceInfo surface_info);

private:
  const sk_sp<GrContext> context;
  const std::shared_ptr<WREOpenGL::TexturePool> texture_pool;
  const int surface_width;
  const int surface_height;
  std::vector<SurfaceInfo> available_surfaces;
  std::vector<SurfaceInfo> used_surfaces;
};
}  // namespace WRESkiaRendering
