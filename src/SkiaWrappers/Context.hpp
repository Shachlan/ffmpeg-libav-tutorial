namespace WRERendering {
class TextRenderer;
}
namespace WREOpenGL {
class TexturePool;
}

namespace WRESkiaRendering {
class Context {
public:
  Context(shared_ptr<WREOpenGL::TexturePool> texture_pool, int width, int height);
  unique_ptr<WRERendering::TextRenderer> get_text_renderer();

private:
  class Impl;
  unique_ptr<Impl> impl;
};
}  // namespace WRESkiaRendering
