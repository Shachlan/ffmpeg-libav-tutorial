// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "TextRenderer.hpp"

#include <core/SkFont.h>
#include <core/SkTextBlob.h>
#include <core/SkTypeface.h>
#include <core/SkSurface.h>
#include <gpu/GrContext.h>

#include "SkiaException.hpp"
#include "SurfacePool.hpp"
#include "TypefaceFactory.hpp"

using WRESkiaRendering::TextRenderer;

TextRenderer::TextRenderer(shared_ptr<TypefaceFactory> typeface_factory,
                           shared_ptr<Surface> surface)
    : surface(surface), typeface_factory(typeface_factory) {
}

uint32_t TextRenderer::render_text(string text, TextRenderConfiguration configuration) {
  surface->context->resetContext();
  auto canvas = surface->surface->getCanvas();
  auto text_color = SkColor4f::FromColor(
      SkColorSetARGB(configuration.text_color[3], configuration.text_color[0],
                     configuration.text_color[1], configuration.text_color[2]));
  SkPaint paint2(text_color);
  auto typeface = typeface_factory->get_typeface(configuration.font_name);
  if (typeface == nullptr) {
    throw SkiaException("Typeface not found: %s", configuration.font_name.c_str());
  }

  auto text_blob =
      SkTextBlob::MakeFromString(text.c_str(), SkFont(typeface, configuration.font_size));
  canvas->drawTextBlob(text_blob.get(), configuration.xCoord, configuration.yCoord, paint2);
  canvas->flush();

  return surface->backing_texture->name;
}
