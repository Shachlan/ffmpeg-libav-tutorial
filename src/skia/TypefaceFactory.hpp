// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include <core/SkRefCnt.h>
#include <unordered_map>

class SkTypeface;

namespace wre_skia {
class TypefaceFactory {
public:
  sk_sp<SkTypeface> get_typeface(std::string typeface_file_name);

private:
  std::unordered_map<std::string, sk_sp<SkTypeface>> file_to_typeface_mapping;
};
}  // namespace wre_skia
