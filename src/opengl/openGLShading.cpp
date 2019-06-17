#include "openGLShading.hpp"

#include "RenderConfiguration.hpp"

#if FRONTEND == 1
#include <emscripten.h>
extern "C" {
#include "html5.h"
}
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>

#define PIXEL_FORMAT GL_RGBA

#else

#include <GLFW/glfw3.h>
#include <GrContext.h>
#include <OpenGL/gl3.h>

#include <stdio.h>
#include <stdlib.h>

#define PIXEL_FORMAT GL_RGB

#endif

#include <SkFont.h>
#include <SkGraphics.h>
#include <SkSurface.h>
#include <SkTextBlob.h>
#include <SkTypeface.h>
#include <math.h>
#include <modules/skottie/include/Skottie.h>
#include <src/core/SkMakeUnique.h>
#include <src/core/SkOSFile.h>
#include <src/gpu/gl/GrGlDefines.h>
#include <src/utils/SkOSPath.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>

#include "GLException.hpp"
#include "ProgramPool.hpp"
#include "SkiaWrappers/SurfacePool.hpp"
#include "SkiaWrappers/TextRenderer.hpp"
#include "SkiaWrappers/TypefaceFactory.hpp"
#include "TextureAllocator.hpp"

using namespace WREOpenGL;
using WRESkiaRendering::SurfacePool;

static ProgramPool program_pool;

static sk_sp<GrContext> skiaContext;
static shared_ptr<WRESkiaRendering::SurfaceInfo> text_surface = nullptr;
static shared_ptr<WRESkiaRendering::SurfaceInfo> lottie_surface = nullptr;
static shared_ptr<SurfacePool> surface_pool = nullptr;
static shared_ptr<WRESkiaRendering::TypefaceFactory> typeface_factory =
    std::make_shared<WRESkiaRendering::TypefaceFactory>();

typedef struct {
  GLuint position_buffer;
  GLuint texture_buffer;
  GLuint vertex_array;
} ProgramInfo;

ProgramInfo blend_program;
#if FRONTEND == 0
GLFWwindow *window;
#endif

GLuint vertex_array;
static sk_sp<skottie::Animation> anim;
static shared_ptr<TextureAllocator> texture_pool = std::make_shared<NaiveTextureAllocator>();

#if FRONTEND == 1

static const float position[12] = {-1.0f, 1.0f,  1.0f, 1.0f, -1.0f, -1.0f,
                                   -1.0f, -1.0f, 1.0f, 1.0f, 1.0f,  -1.0f};

#else

static const float position[12] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
                                   -1.0f, 1.0f,  1.0f, -1.0f, 1.0f,  1.0f};

#endif

static const float textureCoords[12] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
                                        0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f};

unique_ptr<TextureInfo> get_texture(int width, int height) {
  return TextureAllocator::get_texture_info(width, height, PIXEL_FORMAT, texture_pool);
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

void setupOpenGL(int width, int height, char *canvasName) {
#if FRONTEND == 1
  EmscriptenWebGLContextAttributes attrs;
  attrs.explicitSwapControl = 0;
  attrs.depth = 1;
  attrs.stencil = 1;
  attrs.antialias = 1;
  attrs.majorVersion = 1;
  attrs.minorVersion = 0;
  attrs.enableExtensionsByDefault = true;

  int emscripten_context = emscripten_webgl_create_context(canvasName, &attrs);
  emscripten_webgl_make_context_current(emscripten_context);

#else

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
#endif

  glViewport(0, 0, width, height);
  blend_program = build_blend_program();

  skiaContext = GrContext::MakeGL();
  surface_pool = std::make_unique<SurfacePool>(skiaContext, texture_pool, width, height);
  text_surface = surface_pool->get_surface_info();
  lottie_surface = surface_pool->get_surface_info();
  log_info("textures: %u, %u", text_surface->backing_texture->name,
           lottie_surface->backing_texture->name);

  SkAutoGraphics ag;

  anim = skottie::Animation::Builder().makeFromFile("data.json");
}

void blendFrames(uint32_t texture1ID, uint32_t texture2ID, float blend_ratio) {
  // log_debug("start blend");
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
  // log_debug("end blend");
}

void getCurrentResults(int width, int height, uint8_t *outputBuffer) {
  glBindVertexArray(0);
  glReadPixels(0, 0, width, height, PIXEL_FORMAT, GL_UNSIGNED_BYTE, outputBuffer);
  surface_pool->release_surface(*text_surface.get());
  surface_pool->release_surface(*lottie_surface.get());
  text_surface = surface_pool->get_surface_info();
  lottie_surface = surface_pool->get_surface_info();
}

uint32_t render_text(string text, int xCoord, int yCoord, int font_size) {
  glBindVertexArray(0);
  auto text_renderer = WRESkiaRendering::TextRenderer(typeface_factory, text_surface);
  skiaContext->resetContext();
  auto configuration = WRESkiaRendering::TextRenderConfiguration{
      "./fonts/pacifico/Pacifico.ttf", font_size, xCoord, yCoord, {255, 0, 0, 255}};

  return text_renderer.render_text(text, configuration);
}

void tearDownOpenGL() {
  glDeleteBuffers(1, &blend_program.position_buffer);
  glDeleteBuffers(1, &blend_program.texture_buffer);
  program_pool.clear();
  glfwDestroyWindow(window);
}

uint32_t render_lottie(double time) {
  // log_debug("start render lottie");
  double intpart;

  auto fractpart = modf(time, &intpart);

  glBindVertexArray(0);
  skiaContext->resetContext();

  auto canvas = lottie_surface->surface->getCanvas();

  {
    SkAutoCanvasRestore acr(canvas, true);
    anim->seek(fractpart);

    canvas->concat(SkMatrix::MakeRectToRect(
        SkRect::MakeSize(anim->size()), SkRect::MakeIWH(256, 256), SkMatrix::kCenter_ScaleToFit));

    canvas->clear(SK_ColorTRANSPARENT);

    anim->render(canvas);
    canvas->flush();
  }

  // log_debug("end render text");
  return lottie_surface->backing_texture->name;
}
