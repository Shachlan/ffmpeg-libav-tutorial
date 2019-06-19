// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include <core/SkRefCnt.h>
#include <unordered_map>

class SkTypeface;

namespace WRESkiaRendering {
class TypefaceFactory {
public:
  sk_sp<SkTypeface> get_typeface(string typeface_file_name);

private:
  std::unordered_map<string, sk_sp<SkTypeface>> file_to_typeface_mapping;
};
}  // namespace WRESkiaRendering
