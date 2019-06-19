#include "SkiaWrappers/Context.hpp"

#include <GrContext.h>
#include <SkSurface.h>
#include <SkTypeface.h>

#include "SkiaWrappers/SurfacePool.hpp"
#include "SkiaWrappers/TypefaceFactory.hpp"

using namespace WRESkiaRendering;

class Context::Impl {
public:
  Impl(sk_sp<GrContext> context, int width, int height)
      : context(context),
        surface_pool(std::make_shared<SurfacePool>(context, width, height)),
        typeface_factory(std::make_shared<TypefaceFactory>()) {
  }

private:
  const sk_sp<GrContext> context;
  const shared_ptr<SurfacePool> surface_pool;
  const shared_ptr<TypefaceFactory> typeface_factory;
};

Context::Context(int width, int height)
    : impl(std::make_unique<Context::Impl>(GrContext::MakeGL(), width, height)) {
}

unique_ptr<WRERendering::TextRenderer> Context::get_text_renderer() {
  return nullptr;
}
