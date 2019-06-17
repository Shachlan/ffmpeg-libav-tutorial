// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "TextureAllocator.hpp"

#include "OpenGLHeaders.hpp"
#include "TextureInfo.hpp"

using namespace WREOpenGL;

unique_ptr<TextureInfo> TextureAllocator::get_texture_info(
    int width, int height, uint32_t format, std::shared_ptr<TextureAllocator> allocator) {
  return std::make_unique<TextureInfo>(allocator->get_texture(width, height, format), width, height,
                                       format, allocator);
}

GLuint NaiveTextureAllocator::get_texture(int width, int height, GLenum format) {
  log_info("create texture");
  GLuint texture_loc;
  glGenTextures(1, &texture_loc);
  GLCheckDbg("Texture generation.");
  glActiveTexture(GL_TEXTURE0 + texture_loc);

  glBindTexture(GL_TEXTURE_2D, texture_loc);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);
  GLCheckDbg("Texture setup.");
  glBindTexture(GL_TEXTURE_2D, 0);
  return texture_loc;
}

void NaiveTextureAllocator::release_texture(TextureInfo &texture) {
  glDeleteTextures(1, &texture.name);
}
