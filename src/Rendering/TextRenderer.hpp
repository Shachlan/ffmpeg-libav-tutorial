namespace WRERendering {
struct TextRenderConfiguration {
  string font_name;
  int font_size;
};

class TextRenderer {
  virtual uint32_t render_text(string text, TextRenderConfiguration configuration) = 0;
};
}  // namespace WRERendering
