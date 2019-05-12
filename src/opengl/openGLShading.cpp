#include "openGLShading.hpp"

#include <string>

#if FRONTEND == 1
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#define PIXEL_FORMAT GL_RGBA

#else

#include <OpenGL/gl3.h>

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#define PIXEL_FORMAT GL_RGB

#endif

#include "ProgramPool.hpp"
#include "TexturePool.hpp"

using namespace WREOpenGL;

static ProgramPool program_pool;
static TexturePool texture_pool;

typedef struct {
  GLuint position_buffer;
  GLuint texture_buffer;
} ProgramInfo;

ProgramInfo invert_program;
ProgramInfo blend_program;
#if FRONTEND == 0
GLFWwindow *window;
#endif

static const float position[12] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
                                   -1.0f, 1.0f,  1.0f, -1.0f, 1.0f,  1.0f};

static const float textureCoords[12] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
                                        0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f};

uint32_t get_texture() {
  return texture_pool.get_texture();
}

static GLuint position_buffer_setup(GLuint program) {
  GLuint positionBuf;
  glGenBuffers(1, &positionBuf);
  glBindBuffer(GL_ARRAY_BUFFER, positionBuf);
  glBufferData(GL_ARRAY_BUFFER, sizeof(position), position, GL_STATIC_DRAW);

  GLint loc = glGetAttribLocation(program, "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, 0);
  return positionBuf;
}

static GLuint texture_buffer_setup(GLuint program) {
  GLuint texturesBuffer;
  glGenBuffers(1, &texturesBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, texturesBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);

  GLint loc = glGetAttribLocation(program, "texCoord");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, 0);
  return texturesBuffer;
}

GLuint get_invert_program() {
  return program_pool.get_program("passthrough", "invert");
}

ProgramInfo build_invert_program() {
  GLuint program = get_invert_program();
  glUseProgram(program);
  GLuint position_buffer = position_buffer_setup(program);
  GLuint texture_buffer = texture_buffer_setup(program);
  return (ProgramInfo){position_buffer, texture_buffer};
}

GLuint get_blend_program() {
  return program_pool.get_program("passthrough", "blend");
}

ProgramInfo build_blend_program() {
  GLuint program = get_blend_program();
  glUseProgram(program);
  GLuint position_buffer = position_buffer_setup(program);
  GLuint texture_buffer = texture_buffer_setup(program);
  return (ProgramInfo){position_buffer, texture_buffer};
}

void setupOpenGL(int width, int height, char *canvasName) {
#if FRONTEND == 1
  EmscriptenWebGLContextAttributes attrs;
  attrs.explicitSwapControl = 0;
  attrs.depth = 1;
  attrs.stencil = 1;
  attrs.antialias = 1;
  attrs.majorVersion = 1;
  attrs.minorVersion = 0;

  int context = emscripten_webgl_create_context(canvasName, &attrs);
  emscripten_webgl_make_context_current(context);

#else

  glfwInit();
  glfwWindowHint(GLFW_VISIBLE, 0);
  window = glfwCreateWindow(width, height, "", NULL, NULL);
  glfwMakeContextCurrent(window);

#endif
  glViewport(0, 0, width, height);
  invert_program = build_invert_program();
  blend_program = build_blend_program();
}

void loadTexture(uint32_t textureID, int width, int height, const uint8_t *buffer) {
  glBindTexture(GL_TEXTURE_2D, textureID);
  glActiveTexture(GL_TEXTURE0 + textureID);
  glTexImage2D(GL_TEXTURE_2D, 0, PIXEL_FORMAT, width, height, 0, PIXEL_FORMAT, GL_UNSIGNED_BYTE,
               buffer);
}

void invertFrame(uint32_t textureID) {
  auto program = get_invert_program();
  glUseProgram(program);
  glUniform1i(glGetUniformLocation(program, "tex"), textureID);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

void blendFrames(uint32_t texture1ID, uint32_t texture2ID, float blend_ratio) {
  auto program = get_blend_program();
  glUseProgram(program);
  glUniform1f(glGetUniformLocation(program, "opacity1"), 1);
  glUniform1f(glGetUniformLocation(program, "opacity2"), blend_ratio);
  glUniform1i(glGetUniformLocation(program, "numberOfLayers"), 2);
  glUniform1i(glGetUniformLocation(program, "blendMode2"), 2);
  glUniform1i(glGetUniformLocation(program, "tex1"), texture1ID);
  glUniform1i(glGetUniformLocation(program, "tex2"), texture2ID);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

void getCurrentResults(int width, int height, uint8_t *outputBuffer) {
  glReadPixels(0, 0, width, height, PIXEL_FORMAT, GL_UNSIGNED_BYTE, outputBuffer);
}

void tearDownOpenGL() {
  glDeleteBuffers(1, &invert_program.position_buffer);
  glDeleteBuffers(1, &invert_program.texture_buffer);
  glDeleteBuffers(1, &blend_program.position_buffer);
  glDeleteBuffers(1, &blend_program.texture_buffer);
  log_debug("releasing programs");
  program_pool.clear();
#if FRONTEND == 0
  log_debug("releasing textures");
  texture_pool.clear();
  glfwDestroyWindow(window);
#endif
}
