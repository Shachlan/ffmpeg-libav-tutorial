// #include "Context.hpp"

// #include <GrContext.h>
// #include <SkSurface.h>
// #include <SkTypeface.h>

// #include "TypefaceFactory.hpp"

// using namespace wre_skia;

// class Context::Impl {
// public:
//   Impl(sk_sp<GrContext> context, int width, int height)
//       : context(context),
//         surface_pool(std::make_shared<SurfacePool>(context, width, height)),
//         typeface_factory(std::make_shared<TypefaceFactory>()) {}

// private:
//   const sk_sp<GrContext> context;
//   const std::shared_ptr<TypefaceFactory> typeface_factory;
// };

// Context::Context(int width, int height)
//     : impl(std::make_unique<Context::Impl>(GrContext::MakeGL(), width, height)) {}

// std::unique_ptr<WRERendering::TextRenderer> Context::get_text_renderer() {
//   return nullptr;
// }
