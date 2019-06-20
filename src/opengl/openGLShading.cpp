#include "openGLShading.hpp"

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
#include <src/gpu/gl/GrGLDefines.h>
#include <src/utils/SkOSPath.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>

#include "DrawCall.hpp"
#include "GLException.hpp"
#include "ProgramPool.hpp"
#include "SkiaWrappers/SurfacePool.hpp"
#include "SkiaWrappers/TextRenderer.hpp"
#include "SkiaWrappers/TypefaceFactory.hpp"

using namespace wre_opengl;
using WRESkiaRendering::SurfacePool;

static ProgramPool program_pool;

static sk_sp<GrContext> skiaContext;
static shared_ptr<WRESkiaRendering::Surface> text_surface;
static shared_ptr<WRESkiaRendering::Surface> lottie_surface;
static SurfacePool *surface_pool;
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

#if FRONTEND == 1

static const float position[12] = {-1.0f, 1.0f,  1.0f, 1.0f, -1.0f, -1.0f,
                                   -1.0f, -1.0f, 1.0f, 1.0f, 1.0f,  -1.0f};

#else

static const float position[12] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
                                   -1.0f, 1.0f,  1.0f, -1.0f, 1.0f,  1.0f};

#endif

static const float textureCoords[12] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
                                        0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f};

static unique_ptr<Texture> texture = nullptr;

GLuint get_texture() {
  return texture->name;
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
  return positionBuf;
}

static GLuint texture_buffer_setup(GLuint program, string buffer_name) {
  GLuint texturesBuffer;
  glGenBuffers(1, &texturesBuffer);
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
  surface_pool = new SurfacePool(skiaContext, width, height);
  text_surface = surface_pool->get_surface();
  lottie_surface = surface_pool->get_surface();
  log_info("textures: %u, %u", text_surface->backing_texture->name,
           lottie_surface->backing_texture->name);

  SkAutoGraphics ag;

  anim = skottie::Animation::Builder().makeFromFile("data.json");

  texture =
      Texture::make_texture(surface_pool->surface_width, surface_pool->surface_height, GL_RGB);
}

void loadTexture(uint32_t texture_name, int width, int height, const uint8_t *buffer) {
  glActiveTexture(GL_TEXTURE0 + texture_name);
  glBindTexture(GL_TEXTURE_2D, texture_name);
  glTexImage2D(GL_TEXTURE_2D, 0, PIXEL_FORMAT, width, height, 0, PIXEL_FORMAT, GL_UNSIGNED_BYTE,
               buffer);
}

void blendFrames(uint32_t texture1ID, uint32_t texture2ID, float blend_ratio) {
  DrawCall call;

  call._frameBuffer = 0;
  call._clearColor = RgbaColor(0, 0, 0, 0);
  auto program = get_blend_program();
  call._program = program;
  call._vertexArray = blend_program.vertex_array;
  call._floatUniforms.emplace_back(glGetUniformLocation(program, "blendFactor"), blend_ratio);
  call._textures.emplace_back(texture1ID, glGetUniformLocation(program, "tex1"));
  call._textures.emplace_back(texture2ID, glGetUniformLocation(program, "tex2"));
  GLint loc = glGetAttribLocation(program, "position");
  call._attributes.emplace_back(loc, blend_program.position_buffer, 2, GL_FLOAT, GL_FALSE, 0,
                                nullptr);

  loc = glGetAttribLocation(program, "texCoord");
  call._attributes.emplace_back(loc, blend_program.texture_buffer, 2, GL_FLOAT, GL_FALSE, 0,
                                nullptr);

  call._arrayBuffers.emplace_back(blend_program.position_buffer, GL_ARRAY_BUFFER, sizeof(position),
                                  (void *)position, GL_STATIC_DRAW);
  call._arrayBuffers.emplace_back(blend_program.texture_buffer, GL_ARRAY_BUFFER,
                                  sizeof(textureCoords), (void *)textureCoords, GL_STATIC_DRAW);

  call._numberOfTrianglesToDraw = 6;

  call.draw();
}

void getCurrentResults(int width, int height, uint8_t *outputBuffer) {
  glBindVertexArray(0);
  glReadPixels(0, 0, width, height, PIXEL_FORMAT, GL_UNSIGNED_BYTE, outputBuffer);
  surface_pool->release_surface(*text_surface.get());
  surface_pool->release_surface(*lottie_surface.get());
  text_surface = surface_pool->get_surface();
  auto canvas = text_surface->surface->getCanvas();
  canvas->clear(SkColorSetARGB(255, 0, 0, 0));

  lottie_surface = surface_pool->get_surface();
  canvas = lottie_surface->surface->getCanvas();
  canvas->clear(SkColorSetARGB(255, 0, 0, 0));
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
