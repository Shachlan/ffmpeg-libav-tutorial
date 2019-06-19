namespace WRERendering {
class TextRenderer;
}

namespace WRESkiaRendering {
class Context {
public:
  Context(int width, int height);
  unique_ptr<WRERendering::TextRenderer> get_text_renderer();

private:
  class Impl;
  unique_ptr<Impl> impl;
};
}  // namespace WRESkiaRendering
