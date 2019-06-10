#include "SkiaWrappers/TextRenderer.hpp"

#include <GrContext.h>
#include <SkFont.h>
#include <SkSurface.h>
#include <SkTextBlob.h>
#include <SkTypeface.h>

#include "SkiaWrappers/SkiaException.hpp"
#include "SkiaWrappers/SurfacePool.hpp"

using WRESkiaRendering::TextRenderer;

TextRenderer::TextRenderer(shared_ptr<SurfacePool> surface_pool,
                           shared_ptr<TypefaceFactory> typeface_factory, sk_sp<GrContext> context)
    : context(context),
      surface_pool(surface_pool),
      typeface_factory(typeface_factory),
      surface(surface_pool->get_surface()) {
}

TextRenderer::~TextRenderer() {
  surface_pool->release_surface(surface);
}

uint32_t TextRenderer::render_text(string text,
                                   WRERendering::TextRenderConfiguration configuration) {
  context->resetContext();
  auto canvas = surface.surface->getCanvas();
  canvas->clear(SkColorSetARGB(255, 0, 0, 0));
  auto text_color = SkColor4f::FromColor(
      SkColorSetARGB(configuration.text_color[1], configuration.text_color[2],
                     configuration.text_color[3], configuration.text_color[0]));
  SkPaint paint2(text_color);
  auto typeface = typeface_factory.get_typeface(configuration.font_name);
  if (typeface == nullptr) {
    throw SkiaException("unknown typeface: " + configuration.font_name);
  }

  auto text_blob =
      SkTextBlob::MakeFromString(text.c_str(), SkFont(typeface, configuration.font_size));
  canvas->drawTextBlob(text_blob.get(), 20, 20, paint2);
  canvas->flush();

  // log_debug("end render text");
  return surface.backing_texture_name;
}
