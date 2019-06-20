#include "DrawCall.hpp"

using namespace wre_opengl;

void DrawCall::draw() {
  GLCheckDbg("Draw beginning.");
  if (_vertexArray) {
    glBindVertexArray(*_vertexArray);
    GLCheckDbg("Bind vertex array.");
  }

  if (_program) {
    glUseProgram(*_program);
    GLCheckDbg("Use program.");
  }

  if (_frameBuffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, *_frameBuffer);
    GLCheckDbg("Bind framebuffer.");
  }

  if (_clearColor) {
    glClearColor(_clearColor->_r, _clearColor->_g, _clearColor->_b, _clearColor->_a);
    glClear(GL_COLOR_BUFFER_BIT);
    GLCheckDbg("Clear color.");
  }

  for (const auto &buffer : _arrayBuffers) {
    glBindBuffer(GL_ARRAY_BUFFER, buffer._buffer);
    glBufferData(GL_ARRAY_BUFFER, buffer._size, buffer._data, buffer._usage);
    GLCheckDbg("Setup buffer.");
  }

  for (const auto &attribute : _attributes) {
    glBindBuffer(GL_ARRAY_BUFFER, attribute._bufferIndex);
    glEnableVertexAttribArray(attribute._index);
    glVertexAttribPointer(attribute._index, attribute._size, attribute._type, attribute._normalized,
                          attribute._stride, attribute._data);
    GLCheckDbg("Setup attribute.");
  }

  GLenum index = 0;
  for (const auto &texture : _textures) {
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D, texture._name);
    glUniform1i(texture._uniform, index);
    GLCheckDbg("Bind texture.");
    ++index;
  }

  for (const auto &uniform : _intUniforms) {
    glUniform1i(uniform._index, uniform._value);
    GLCheckDbg("Set int uniform.");
  }

  for (const auto &uniform : _floatUniforms) {
    glUniform1f(uniform._index, uniform._value);
    GLCheckDbg("Set float uniform.");
  }

  glDrawArrays(GL_TRIANGLES, 0, _numberOfTrianglesToDraw);
  GLCheckDbg("Draw.");
}
