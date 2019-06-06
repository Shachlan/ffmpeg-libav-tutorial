#include "Rendering/TextRenderer.hpp"

#include <SkRefCnt.h>
#include <vector>

class TexturePool;
class SkSurface;
class GrContext;

namespace WRESkiaRendering {
class SkiaSurfacePool {
public:
  SkiaSurfacePool(sk_sp<GrContext> context, std::shared_ptr<TexturePool> texture_pool,
                  int surface_width, int surface_height);
  sk_sp<SkSurface> get_surface();

private:
  const sk_sp<GrContext> context;
  const std::shared_ptr<TexturePool> texture_pool;
  const int surface_width;
  const int surface_height;
  std::vector<sk_sp<SkSurface>> available_surfaces;
  std::vector<sk_sp<SkSurface>> used_surfaces;
};
}  // namespace WRESkiaRendering
