// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include <unordered_set>

namespace WREOpenGL {
struct TextureInfo;

class TextureAllocator {
public:
  static unique_ptr<TextureInfo> get_texture_info(int width, int height, uint32_t format,
                                                  std::shared_ptr<TextureAllocator> allocator);
  virtual void release_texture(TextureInfo &texture) = 0;
  virtual ~TextureAllocator() = default;

protected:
  virtual uint32_t get_texture(int width, int height, uint32_t format) = 0;
};

class NaiveTextureAllocator : public TextureAllocator {
public:
  void release_texture(TextureInfo &texture) override;

protected:
  uint32_t get_texture(int width, int height, uint32_t format) override;
};
}  // namespace WREOpenGL
