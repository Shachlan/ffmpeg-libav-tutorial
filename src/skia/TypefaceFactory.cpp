// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "TypefaceFactory.hpp"

#include <core/SkTypeface.h>

sk_sp<SkTypeface> wre_skia::TypefaceFactory::get_typeface(std::string typeface_file_name) {
  auto iter = file_to_typeface_mapping.find(typeface_file_name);

  if (iter != file_to_typeface_mapping.end()) {
    return (*iter).second;
  }

  auto typeface = SkTypeface::MakeFromFile(typeface_file_name.c_str());
  file_to_typeface_mapping[typeface_file_name] = typeface;
  return typeface;
}
