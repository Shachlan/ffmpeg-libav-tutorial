#include "SkiaWrappers/TextRenderer.hpp"

#include <GrContext.h>
#include <SkFont.h>
#include <SkSurface.h>
#include <SkTextBlob.h>
#include <SkTypeface.h>

#include "SkiaWrappers/SkiaException.hpp"
#include "SkiaWrappers/SurfacePool.hpp"
#include "SkiaWrappers/TypefaceFactory.hpp"

using WRESkiaRendering::TextRenderer;

TextRenderer::TextRenderer(shared_ptr<TypefaceFactory> typeface_factory,
                           shared_ptr<SurfaceInfo> surface_info)
    : surface_info(std::move(surface_info)), typeface_factory(typeface_factory) {
}

uint32_t TextRenderer::render_text(string text, TextRenderConfiguration configuration) {
  surface_info->context->resetContext();
  auto canvas = surface_info->surface->getCanvas();
  auto text_color = SkColor4f::FromColor(
      SkColorSetARGB(configuration.text_color[3], configuration.text_color[0],
                     configuration.text_color[1], configuration.text_color[2]));
  SkPaint paint2(text_color);
  auto typeface = typeface_factory->get_typeface(configuration.font_name);
  if (typeface == nullptr) {
    throw SkiaException("unknown typeface: " + configuration.font_name);
  }

  auto text_blob =
      SkTextBlob::MakeFromString(text.c_str(), SkFont(typeface, configuration.font_size));
  canvas->drawTextBlob(text_blob.get(), configuration.xCoord, configuration.yCoord, paint2);
  canvas->flush();

  return surface_info->backing_texture->name;
}
