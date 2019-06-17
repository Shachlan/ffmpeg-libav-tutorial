#pragma once

namespace WREOpenGL {
class TextureAllocator;

struct TextureInfo {
  TextureInfo(uint32_t name, int width, int height, uint32_t format,
              std::shared_ptr<TextureAllocator> allocator)
      : name(name), width(width), height(height), format(format), allocator(allocator) {
  }
  TextureInfo &operator=(const TextureInfo &other) = delete;
  TextureInfo(const TextureInfo &other) = delete;
  TextureInfo &operator=(TextureInfo &&other) = default;
  TextureInfo(TextureInfo &&other) = default;
  ~TextureInfo();

  void load(const uint8_t *buffer);

  const uint32_t name;
  const int width;
  const int height;
  const uint32_t format;

private:
  const std::shared_ptr<TextureAllocator> allocator;
};
}  // namespace WREOpenGL
