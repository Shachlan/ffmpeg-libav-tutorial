#include "TypefaceFactory.hpp"

#include <SkTypeface.h>

sk_sp<SkTypeface> WRESkiaRendering::TypefaceFactory::get_typeface(string typeface_file_name) {
  auto iter = file_to_typeface_mapping.find(typeface_file_name);

  if (iter == file_to_typeface_mapping.end()) {
    auto typeface = SkTypeface::MakeFromFile(typeface_file_name.c_str());
    iter = file_to_typeface_mapping.insert(typeface).first;
  }

  return *iter;
}