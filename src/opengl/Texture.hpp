// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#pragma once

namespace wre_opengl {
/// RAII wrapper for an OpenGL texture, with a set size and format. Since OpenGL is not thread-safe,
/// usage of this struct on several threads isn't safe, either. A texture should only be used in the
/// OpenGL context it was created.
struct Texture {
  Texture(int width, int height, uint32_t format);
  Texture &operator=(const Texture &other) = delete;
  Texture(const Texture &other) = delete;
  Texture &operator=(Texture &&other) = default;
  Texture(Texture &&other) = default;
  ~Texture();

  /// Loads the given \c buffer into the wrapped OpenGL texture.
  void load(const uint8_t *buffer);

  /// OpenGL name of the wrapped texture.
  const uint32_t _name;
  const int _width;
  const int _height;
  const uint32_t _format;
};
}  // namespace wre_opengl
