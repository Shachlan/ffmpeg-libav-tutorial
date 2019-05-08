#pragma once

#include <vector>

namespace WREOpenGL {

#ifdef DEBUG
#define GLCheckDbg(format, ...) check_gl_errors(format, ##__VA_ARGS__)
#else
#define GLCheckDbg(format, ...)
#endif

/// Checks whether OpenGL contains errors, and throws a\c GLException if there are any.
/// The thrown exception's description will contain the OpenGL errors, and \c format and the
/// additional arguments.
void check_gl_errors(string format, ...);

/// Throws a \c GLException with a full description containing \c format and the
/// additional arguments.
void throw_gl_exception(string format, ...);

class GLException : public std::exception {
public:
  /// Error message of the exception.
  const char *what() const noexcept {
    return full_description;
  }

  /// Description of the error that caused the exception.
  char *full_description;
};

}  // namespace WREOpenGL
