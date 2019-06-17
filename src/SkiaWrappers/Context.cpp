#include "SkiaWrappers/Context.hpp"

#include <GrContext.h>
#include <SkSurface.h>
#include <SkTypeface.h>

#include "SkiaWrappers/SurfacePool.hpp"
#include "SkiaWrappers/TypefaceFactory.hpp"
#include "opengl/TextureAllocator.hpp"

using namespace WRESkiaRendering;
using WREOpenGL::TextureAllocator;

class Context::Impl {
public:
  Impl(sk_sp<GrContext> context, shared_ptr<TextureAllocator> texture_allocator, int width,
       int height)
      : context(context),
        surface_pool(std::make_unique<SurfacePool>(context, texture_allocator, width, height)),
        typeface_factory(std::make_shared<TypefaceFactory>()) {
  }

private:
  const sk_sp<GrContext> context;
  const shared_ptr<SurfacePool> surface_pool;
  const shared_ptr<TypefaceFactory> typeface_factory;
};

Context::Context(shared_ptr<TextureAllocator> texture_allocator, int width, int height)
    : impl(std::make_unique<Context::Impl>(GrContext::MakeGL(), texture_allocator, width, height)) {
}

unique_ptr<WRERendering::TextRenderer> Context::get_text_renderer() {
  return nullptr;
}
