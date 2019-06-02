/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <OpenGL/gl3.h>
#include <src/gpu/gl/GrGlDefines.h>
#include "GrContext.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkPictureRecorder.h"
#include "include/core/SkStream.h"
#include "include/core/SkSurface.h"
#include "modules/skottie/include/Skottie.h"
#include "src/core/SkMakeUnique.h"
#include "src/core/SkOSFile.h"
#include "src/utils/SkOSPath.h"

#include <GLFW/glfw3.h>
#include <vector>

namespace {

class Sink {
public:
  virtual ~Sink() = default;
  Sink(const Sink &) = delete;
  Sink &operator=(const Sink &) = delete;

  bool handleFrame(const sk_sp<skottie::Animation> &anim, size_t idx) const {
    const auto frame_file = SkStringPrintf("0%06d.%s", idx, fExtension.c_str());
    SkFILEWStream stream(SkOSPath::Join("./", frame_file.c_str()).c_str());

    if (!stream.isValid()) {
      SkDebugf("Could not open '%s/%s' for writing.\n", "./", frame_file.c_str());
      return false;
    }

    return this->saveFrame(anim, &stream);
  }

protected:
  Sink(const char *ext) : fExtension(ext) {
  }

  virtual bool saveFrame(const sk_sp<skottie::Animation> &anim, SkFILEWStream *) const = 0;

private:
  const SkString fExtension;
};

static sk_sp<GrContext> skiaContext;
GLFWwindow *window;

class PNGSink final : public Sink {
public:
  static const int width = 128;
  static const int height = 128;
  static const int window_width = 1920;
  static const int window_height = 1080;

  PNGSink() : INHERITED("png"), fSurface(SkSurface::MakeRasterN32Premul(128, 128)) {
    if (!glfwInit()) {
      exit(1);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, 0);
    window = glfwCreateWindow(window_width, window_height, "", NULL, NULL);
    if (window == NULL) {
      exit(1);
    }
    glfwMakeContextCurrent(window);
    skiaContext = GrContext::MakeGL();
    uint32_t texture_name;
    glGenTextures(1, &texture_name);
    glBindTexture(GL_TEXTURE_2D, texture_name);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window_width, window_height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    GrGLTextureInfo texture_info = {
        .fID = texture_name, .fTarget = GL_TEXTURE_2D, .fFormat = GR_GL_RGBA8};
    GrBackendTexture texture(window_width, window_height, GrMipMapped::kNo, texture_info);
    fSurface = sk_sp(SkSurface::MakeFromBackendTexture(skiaContext.get(), texture,
                                                       kTopLeft_GrSurfaceOrigin, 0,
                                                       kRGBA_8888_SkColorType, nullptr, nullptr));

    if (!fSurface) {
      SkDebugf("Could not allocate a %d x %d surface.\n", width, height);
    }
  }

  bool saveFrame(const sk_sp<skottie::Animation> &anim, SkFILEWStream *stream) const override {
    if (!fSurface)
      return false;

    auto *canvas = fSurface->getCanvas();
    SkAutoCanvasRestore acr(canvas, true);

    canvas->concat(SkMatrix::MakeRectToRect(SkRect::MakeSize(anim->size()),
                                            SkRect::MakeIWH(width, height),
                                            SkMatrix::kCenter_ScaleToFit));

    canvas->clear(SK_ColorTRANSPARENT);
    anim->render(canvas);

    auto png_data = fSurface->makeImageSnapshot()->encodeToData();
    if (!png_data) {
      SkDebugf("Failed to encode frame!\n");
      return false;
    }

    return stream->write(png_data->data(), png_data->size());
  }

private:
  sk_sp<SkSurface> fSurface;

  using INHERITED = Sink;
};
}  // namespace

int main(int argc, char **argv) {
  SkAutoGraphics ag;

  std::unique_ptr<Sink> sink = skstd::make_unique<PNGSink>();

  auto anim = skottie::Animation::Builder().makeFromFile("gear.json");
  if (!anim) {
    SkDebugf("Could not load animation: '%s'.\n", "gear.json");
    return 1;
  }

  static constexpr double kMaxFrames = 10000;
  const auto t0 = SkTPin(0.0, 0.0, 1.0), t1 = SkTPin(1.0, t0, 1.0),
             advance = 1 / std::min(anim->duration() * 10.0, kMaxFrames);

  size_t frame_index = 0;
  for (auto t = t0; t <= t1; t += advance) {
    printf("seek to %f\n", t);
    anim->seek(t);
    sink->handleFrame(anim, frame_index++);
  }

  return 0;
}
