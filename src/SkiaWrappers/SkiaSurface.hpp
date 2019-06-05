#include "Rendering/TextRenderer.hpp"

using WRERendering::TextRenderer;

class GrContext;
template <class T>
class sk_sp;
template <>
class sk_sp<GrContext>;

namespace WRESkiaRendering {
class SkiaSurface : TextRenderer {
public:
  SkiaSurface(int width, int height, uint32_t texture_name, sk_sp<GrContext> skia_context);
  uint32_t render_text(string text, WRERendering::TextRenderConfiguration configuration) override;

private:
  class Impl;
  const std::unique_ptr<Impl> implementation;
};
}  // namespace WRESkiaRendering
