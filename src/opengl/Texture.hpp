#pragma once

namespace WREOpenGL {
struct Texture {
  static unique_ptr<Texture> make_texture(int width, int height, uint32_t format);

  Texture &operator=(const Texture &other) = delete;
  Texture(const Texture &other) = delete;
  Texture &operator=(Texture &&other) = default;
  Texture(Texture &&other) = default;
  ~Texture();

  /// Loads the given \c buffer into the wrapped OpenGL texture.
  void load(const uint8_t *buffer);

  /// OpenGL name of the wrapped texture.
  const uint32_t name;
  const int width;
  const int height;
  const uint32_t format;

private:
  Texture(uint32_t name, int width, int height, uint32_t format)
      : name(name), width(width), height(height), format(format) {
  }
};
}  // namespace WREOpenGL
