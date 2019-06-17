#include "SkiaWrappers/SurfaceInfo.hpp"

namespace WRESkiaRendering {
class TypefaceFactory;

struct TextRenderConfiguration {
  string font_name;
  int font_size;
  int xCoord;
  int yCoord;
  uint8_t text_color[4];
};

class TextRenderer {
public:
  TextRenderer(shared_ptr<TypefaceFactory> typeface_factory, shared_ptr<SurfaceInfo> surface_info);
  uint32_t render_text(string text, TextRenderConfiguration configuration);

private:
  const shared_ptr<SurfaceInfo> surface_info;
  const shared_ptr<TypefaceFactory> typeface_factory;
};
}  // namespace WRESkiaRendering
