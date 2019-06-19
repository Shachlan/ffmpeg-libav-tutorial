#include "Texture.hpp"

#include "OpenGLHeaders.hpp"

using WREOpenGL::Texture;

Texture::~Texture() {
  glDeleteTextures(1, &name);
  GLCheckDbg("Delete texture.");
}

void Texture::load(const uint8_t *buffer) {
  glActiveTexture(GL_TEXTURE0 + name);
  glBindTexture(GL_TEXTURE_2D, name);
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, buffer);
}

unique_ptr<Texture> Texture::make_texture(int width, int height, uint32_t format) {
  log_info("create texture");
  GLuint texture_name;
  glGenTextures(1, &texture_name);
  GLCheckDbg("Texture generation.");
  glActiveTexture(GL_TEXTURE0 + texture_name);

  glBindTexture(GL_TEXTURE_2D, texture_name);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);
  GLCheckDbg("Texture setup.");
  glBindTexture(GL_TEXTURE_2D, 0);
  GLCheckDbg("Reset texture binding.");

  return unique_ptr<Texture>(new Texture(texture_name, width, height, format));
}
