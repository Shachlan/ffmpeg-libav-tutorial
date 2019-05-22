// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "TexturePool.hpp"

#include "opengl/OpenGLHeaders.hpp"

using namespace WREOpenGL;

GLuint create_texture() {
  log_info("create texture");
  GLuint textureLoc;
  glGenTextures(1, &textureLoc);
  GLCheckDbg("Failed to generate texture.");
  glActiveTexture(GL_TEXTURE0 + textureLoc);

  glBindTexture(GL_TEXTURE_2D, textureLoc);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);
  GLCheckDbg("Failed to setup texture.");
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
