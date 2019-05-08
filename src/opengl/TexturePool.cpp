#include "TexturePool.hpp"

#if FRONTEND == 1
#include <GLES2/gl2.h>
#else
#include <OpenGL/gl3.h>
#endif

using namespace WREOpenGL;

GLuint create_texture() {
  GLuint textureLoc;
  glGenTextures(1, &textureLoc);
  glActiveTexture(GL_TEXTURE0 + textureLoc);

  glBindTexture(GL_TEXTURE_2D, textureLoc);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  return textureLoc;
}

GLuint TexturePool::get_texture() {
  GLuint name;
  if (this->available_textures.empty()) {
    name = create_texture();
  } else {
    name = *this->available_textures.begin();
    this->available_textures.erase(name);
  }

  if (name > 0) {
    used_textures.insert(name);
  }

  return name;
}

void TexturePool::release_texture(GLuint name) {
  this->used_textures.erase(name);
  this->available_textures.insert(name);
}

void TexturePool::flush() {
  for (auto &name : this->available_textures) {
    glDeleteTextures(1, &name);
  }
}

void TexturePool::clear() {
  this->flush();

  for (auto &name : this->used_textures) {
    glDeleteTextures(1, &name);
  }
}
