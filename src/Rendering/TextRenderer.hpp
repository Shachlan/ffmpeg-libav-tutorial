namespace WRERendering {
struct TextRenderConfiguration {
  string font_name;
  int font_size;
  uint8_t text_color[4];
};

class TextRenderer {
  virtual uint32_t render_text(string text, TextRenderConfiguration configuration) = 0;
};
}  // namespace WRERendering
