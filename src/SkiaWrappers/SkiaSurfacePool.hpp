#include "Rendering/TextRenderer.hpp"

#include <SkRefCnt.h>
#include <vector>

class TexturePool;
class SkSurface;

namespace WRESkiaRendering {
class SkiaSurfacePool {
public:
  SkiaSurfacePool(std::shared_ptr<TexturePool> texture_pool, int surface_width, int surface_height);
  sk_sp<SkSurface> get_surface();

private:
  std::shared_ptr<TexturePool> texture_pool;
  int surface_width;
  int surface_height;
  std::vector<sk_sp<SkSurface>> available_surfaces;
  std::vector<sk_sp<SkSurface>> using_surfaces;
};
}  // namespace WRESkiaRendering
