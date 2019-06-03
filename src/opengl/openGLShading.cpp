#include "openGLShading.hpp"

#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>
#include <math.h>
#include <src/gpu/gl/GrGlDefines.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include "GLException.hpp"
#include "GrContext.h"
#include "ProgramPool.hpp"
#include "SkData.h"
#include "SkFont.h"
#include "SkGraphics.h"
#include "SkImage.h"
#include "SkPaint.h"
#include "SkPictureRecorder.h"
#include "SkStream.h"
#include "SkString.h"
#include "SkSurface.h"
#include "SkTextBlob.h"
#include "SkTypeface.h"
#include "gl/GrGLInterface.h"
#include "modules/skottie/include/Skottie.h"
#include "src/core/SkMakeUnique.h"
#include "src/core/SkOSFile.h"
#include "src/utils/SkOSPath.h"

using namespace WREOpenGL;

static ProgramPool program_pool;

static sk_sp<GrContext> skiaContext;
static sk_sp<SkSurface> surface;
static sk_sp<SkSurface> lottie_surface;
GLuint backend_texture;
GLuint lottie_texture;

typedef struct {
  GLuint position_buffer;
  GLuint texture_buffer;
  GLuint vertex_array;
} ProgramInfo;

ProgramInfo blend_program;
GLFWwindow *window;
GLuint vertex_array;
static sk_sp<SkTypeface> typeface;
static sk_sp<skottie::Animation> anim;

static const float position[12] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
                                   -1.0f, 1.0f,  1.0f, -1.0f, 1.0f,  1.0f};

static const float textureCoords[12] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
                                        0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f};

GLuint get_texture() {
  GLuint textureLoc;
  glGenTextures(1, &textureLoc);
  glActiveTexture(GL_TEXTURE0 + textureLoc);

  glBindTexture(GL_TEXTURE_2D, textureLoc);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);
  return textureLoc;
}

static GLuint generate_vertex_array() {
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  return vao;
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

static GLuint texture_buffer_setup(GLuint program, string buffer_name) {
  GLuint texturesBuffer;
  glGenBuffers(1, &texturesBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, texturesBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);

  GLint loc = glGetAttribLocation(program, buffer_name.c_str());
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, 0);
  return texturesBuffer;
}

GLuint get_blend_program() {
  return program_pool.get_program("passthrough", "blend");
}

ProgramInfo build_blend_program() {
  GLuint program = get_blend_program();
  glUseProgram(program);

  auto vertex_array = generate_vertex_array();
  GLuint position_buffer = position_buffer_setup(program);

  GLuint texture_buffer = texture_buffer_setup(program, "texCoord");
  return (ProgramInfo){position_buffer, texture_buffer, vertex_array};
}

static sk_sp<SkSurface> create_surface(int width, int height, GLuint texture_name) {
  glBindTexture(GL_TEXTURE_2D, texture_name);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  GrGLTextureInfo texture_info = {
      .fID = texture_name, .fTarget = GL_TEXTURE_2D, .fFormat = GR_GL_RGBA8};
  GrBackendTexture texture(width, height, GrMipMapped::kNo, texture_info);
  auto surface =
      sk_sp(SkSurface::MakeFromBackendTexture(skiaContext.get(), texture, kTopLeft_GrSurfaceOrigin,
                                              0, kRGBA_8888_SkColorType, nullptr, nullptr));
  if (!surface) {
    printf("SkSurface::MakeRenderTarget returned null\n");
    exit(1);
  }

  return surface;
}

void setupOpenGL(int width, int height) {
  if (!glfwInit()) {
    exit(1);
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_VISIBLE, 0);
  window = glfwCreateWindow(width, height, "", NULL, NULL);
  if (window == NULL) {
    printf("No window\n");
    exit(1);
  }
  glfwMakeContextCurrent(window);

  glViewport(0, 0, width, height);
  blend_program = build_blend_program();

  skiaContext = GrContext::MakeGL();
  backend_texture = get_texture();
  lottie_texture = get_texture();
  surface = create_surface(width, height, backend_texture);
  lottie_surface = create_surface(width, height, lottie_texture);

  SkAutoGraphics ag;

  anim = skottie::Animation::Builder().makeFromFile("data.json");

  typeface = SkTypeface::MakeFromFile("./fonts/pacifico/Pacifico.ttf");
}

void loadTexture(uint32_t texture_name, int width, int height, const uint8_t *buffer) {
  glActiveTexture(GL_TEXTURE0 + texture_name);
  glBindTexture(GL_TEXTURE_2D, texture_name);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
}

void blendFrames(uint32_t texture1ID, uint32_t texture2ID, float blend_ratio) {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  auto program = get_blend_program();
  glUseProgram(program);
  glBindVertexArray(blend_program.vertex_array);
  glUniform1f(glGetUniformLocation(program, "blendFactor"), blend_ratio);

  glActiveTexture(GL_TEXTURE0 + texture1ID);
  glBindTexture(GL_TEXTURE_2D, texture1ID);
  glUniform1i(glGetUniformLocation(program, "tex1"), texture1ID);

  glActiveTexture(GL_TEXTURE0 + texture2ID);
  glBindTexture(GL_TEXTURE_2D, texture2ID);
  glUniform1i(glGetUniformLocation(program, "tex2"), texture2ID);

  glDrawArrays(GL_TRIANGLES, 0, 6);
}

void getCurrentResults(int width, int height, uint8_t *outputBuffer) {
  glBindVertexArray(0);
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, outputBuffer);
}

uint32_t render_text(string text) {
  glBindVertexArray(0);
  skiaContext->resetContext();
  auto canvas = surface->getCanvas();
  canvas->clear(SkColorSetARGB(255, 0, 0, 0));
  auto text_color = SkColor4f::FromColor(SkColorSetARGB(255, 0, 0, 255));
  SkPaint paint2(text_color);
  if (typeface == nullptr) {
    printf("no typeface\n");
    exit(1);
  }

  auto text_blob = SkTextBlob::MakeFromString(text.c_str(), SkFont(typeface, 30));
  canvas->drawTextBlob(text_blob.get(), 20, 20, paint2);
  canvas->flush();

  return backend_texture;
}

void tearDownOpenGL() {
  glDeleteBuffers(1, &blend_program.position_buffer);
  glDeleteBuffers(1, &blend_program.texture_buffer);
  program_pool.clear();
  glfwDestroyWindow(window);
}

uint32_t render_lottie(double time) {
  double intpart;

  auto fractpart = modf(time, &intpart);

  glBindVertexArray(0);
  skiaContext->resetContext();

  auto canvas = lottie_surface->getCanvas();

  {
    SkAutoCanvasRestore acr(canvas, true);
    anim->seek(fractpart);

    canvas->concat(SkMatrix::MakeRectToRect(
        SkRect::MakeSize(anim->size()), SkRect::MakeIWH(256, 256), SkMatrix::kCenter_ScaleToFit));

    canvas->clear(SK_ColorTRANSPARENT);

    anim->render(canvas);
    canvas->flush();
  }

  return lottie_texture;
}
