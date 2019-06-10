#include <SkRefCnt.h>
#include <vector>

namespace WREOpenGL {
class TexturePool;
}
class SkSurface;
class GrContext;

namespace WRESkiaRendering {
struct SkiaSurface {
  SkiaSurface(uint32_t texture_name, sk_sp<SkSurface> surface);
  SkiaSurface() = default;

  uint32_t backing_texture_name;
  sk_sp<SkSurface> surface;
};

class SurfacePool {
public:
  SurfacePool(sk_sp<GrContext> context, std::shared_ptr<WREOpenGL::TexturePool> texture_pool,
              int surface_width, int surface_height);
  ~SurfacePool();
  SkiaSurface get_surface();
  void release_surface(SkiaSurface surface);

private:
  const sk_sp<GrContext> context;
  const std::shared_ptr<WREOpenGL::TexturePool> texture_pool;
  const int surface_width;
  const int surface_height;
  std::vector<SkiaSurface> available_surfaces;
  std::vector<SkiaSurface> used_surfaces;
};
}  // namespace WRESkiaRendering
