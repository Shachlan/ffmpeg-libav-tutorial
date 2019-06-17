#include "TextureInfo.hpp"

#include "OpenGLHeaders.hpp"
#include "TextureAllocator.hpp"

WREOpenGL::TextureInfo::~TextureInfo() {
  allocator->release_texture(*this);
}

void WREOpenGL::TextureInfo::load(const uint8_t *buffer) {
  glActiveTexture(GL_TEXTURE0 + name);
  glBindTexture(GL_TEXTURE_2D, name);
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, buffer);
}
