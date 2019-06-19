// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#pragma once

#include "Base/BaseException.hpp"

namespace WREOpenGL {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#if DEBUG == 1
#define GLCheckDbg(format, ...) check_gl_errors(format, ##__VA_ARGS__)
#else
#define GLCheckDbg(format, ...) empty(format, ##__VA_ARGS__)
#endif
#pragma clang diagnostic pop

/// Checks whether OpenGL contains errors, and throws a\c GLException if there are any.
/// The thrown exception's description will contain the OpenGL errors, and \c format and the
/// additional arguments.
void check_gl_errors(string format, ...);

DefineException(GLException);

}  // namespace WREOpenGL
