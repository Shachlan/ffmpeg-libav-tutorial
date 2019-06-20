#include <optional>
#include <vector>

#include "OpenGLHeaders.hpp"

namespace wre_opengl {
template <class T>
struct Uniform {
  Uniform(GLuint index, T value) : _index(index), _value(value) {}
  GLuint _index;
  T _value;
};

struct RgbaColor {
  RgbaColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a) : _r(r), _g(g), _b(b), _a(a) {}
  GLclampf _r;
  GLclampf _g;
  GLclampf _b;
  GLclampf _a;
};

struct ArrayBuffer {
  ArrayBuffer(GLuint buffer, GLenum target, GLsizeiptr size, void *data, GLenum usage)
      : _buffer(buffer), _size(size), _data(data), _usage(usage) {}
  GLuint _buffer;
  GLsizeiptr _size;
  GLvoid *_data;
  GLenum _usage;
};

struct TextureUniform {
  TextureUniform(GLuint name, GLuint uniform) : _name(name), _uniform(uniform) {}

  GLuint _name;
  GLuint _uniform;
};

struct DrawCall {
  void draw();
  std::optional<GLuint> _vertexArray;
  std::optional<GLuint> _program;
  std::optional<GLuint> _frameBuffer;
  std::optional<RgbaColor> _clearColor;
  std::vector<TextureUniform> _textures;
  std::vector<ArrayBuffer> _arrayBuffers;
  std::vector<Uniform<int>> _intUniforms;
  std::vector<Uniform<float>> _floatUniforms;
  int _numberOfTrianglesToDraw;
};
}  // namespace wre_opengl
