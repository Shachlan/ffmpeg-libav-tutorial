// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "Surface.hpp"

namespace wre_skia {
class TypefaceFactory;

struct TextRenderConfiguration {
  std::string font_name;
  int font_size;
  int xCoord;
  int yCoord;
  uint8_t text_color[4];
};

class TextRenderer {
public:
  TextRenderer(std::shared_ptr<TypefaceFactory> typeface_factory, std::shared_ptr<Surface> surface);
  uint32_t render_text(std::string text, TextRenderConfiguration configuration);

private:
  const std::shared_ptr<Surface> surface;
  const std::shared_ptr<TypefaceFactory> typeface_factory;
};
}  // namespace wre_skia
