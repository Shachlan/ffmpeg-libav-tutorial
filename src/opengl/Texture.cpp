// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "Texture.hpp"

#include "OpenGLHeaders.hpp"

using namespace wre_opengl;

namespace {
GLuint create_texture(int width, int height, uint32_t format) {
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
  return texture_name;
}
}  // namespace

Texture::Texture(int width, int height, GLuint format)
    : _name(create_texture(width, height, format)),
      _width(width),
      _height(height),
      _format(format) {}

Texture::~Texture() {
  glDeleteTextures(1, &_name);
  GLCheckDbg("Delete texture.");
}

void Texture::load(const uint8_t *buffer) {
  glBindTexture(GL_TEXTURE_2D, _name);
  glTexImage2D(GL_TEXTURE_2D, 0, _format, _width, _height, 0, _format, GL_UNSIGNED_BYTE, buffer);
  GLCheckDbg("Load data.");
  glBindTexture(GL_TEXTURE_2D, 0);
  GLCheckDbg("Reset texture binding.");
}
