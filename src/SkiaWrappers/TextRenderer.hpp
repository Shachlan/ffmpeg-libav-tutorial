#include <SkRefCnt.h>

#include "Rendering/TextRenderer.hpp"

class GrContext;
class SkSurface;

namespace WRESkiaRendering {
class TypefaceFactory;
class SurfacePool;
class SkiaSurface;

class TextRenderer : WRERendering::TextRenderer {
public:
  TextRenderer(shared_ptr<SurfacePool> surface_pool, shared_ptr<TypefaceFactory> typeface_factory,
               sk_sp<GrContext> context);
  ~TextRenderer();
  virtual uint32_t render_text(string text,
                               WRERendering::TextRenderConfiguration configuration) override;

private:
  const sk_sp<GrContext> context;
  const unique_ptr<SkiaSurface> surface;
  const shared_ptr<SurfacePool> surface_pool;
  const shared_ptr<TypefaceFactory> typeface_factory;
};
}  // namespace WRESkiaRendering
