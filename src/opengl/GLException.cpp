// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "opengl/GLException.hpp"

#include <stdio.h>

#include "opengl/OpenGLHeaders.hpp"

using std::vector;

namespace WREOpenGL {

static string error_description(GLenum error_code) {
  switch (error_code) {
    case GL_INVALID_ENUM:
      return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
      return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
      return "GL_INVALID_OPERATION";
    case GL_OUT_OF_MEMORY:
      return "GL_OUT_OF_MEMORY";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      return "GL_INVALID_FRAMEBUFFER_OPERATION";
  }

  return "Unknown error code: " + std::to_string(error_code);
}

static string error_descriptions(vector<GLenum> error_codes) {
  string error_descriptions = "Errors: ";
  for (auto &error_code : error_codes) {
    error_descriptions += error_description(error_code) + ",";
  }

  return error_descriptions;
}

static GLException get_exception(string format, va_list argList) {
  GLException exception;
  vasprintf(&exception.full_description, format.c_str(), argList);
  return exception;
}

void throw_gl_exception(string format, ...) {
  va_list argList;
  va_start(argList, format);
  auto exception = get_exception(format, argList);
  va_end(argList);

  throw exception;
}

static vector<GLenum> get_openGL_errors() {
  vector<GLenum> errors;
  GLenum error;

  while ((error = glGetError()) != GL_NO_ERROR) {
    errors.insert(errors.end(), error);
  }

  return errors;
}

void check_gl_errors(string format, ...) {
  auto errors = get_openGL_errors();
  if (errors.size() == 0) {
    return;
  }

  format += error_descriptions(errors);
  va_list argList;
  va_start(argList, format);
  auto exception = get_exception(format, argList);
  va_end(argList);

  throw exception;
}

}  // namespace WREOpenGL
