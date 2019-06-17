namespace WRERendering {
class TextRenderer;
}
namespace WREOpenGL {
class TextureAllocator;
}

namespace WRESkiaRendering {
class Context {
public:
  Context(shared_ptr<WREOpenGL::TextureAllocator> texture_allocator, int width, int height);
  unique_ptr<WRERendering::TextRenderer> get_text_renderer();

private:
  class Impl;
  unique_ptr<Impl> impl;
};
}  // namespace WRESkiaRendering
