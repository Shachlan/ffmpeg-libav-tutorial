/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/core/SkCanvas.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkPictureRecorder.h"
#include "include/core/SkStream.h"
#include "include/core/SkSurface.h"
#include "modules/skottie/include/Skottie.h"
#include "src/core/SkMakeUnique.h"
#include "src/core/SkOSFile.h"
#include "src/utils/SkOSPath.h"

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

class PNGSink final : public Sink {
public:
  PNGSink() : INHERITED("png"), fSurface(SkSurface::MakeRasterN32Premul(128, 128)) {
    if (!fSurface) {
      SkDebugf("Could not allocate a %d x %d surface.\n", 128, 128);
    }
  }

  bool saveFrame(const sk_sp<skottie::Animation> &anim, SkFILEWStream *stream) const override {
    if (!fSurface)
      return false;

    auto *canvas = fSurface->getCanvas();
    SkAutoCanvasRestore acr(canvas, true);

    canvas->concat(SkMatrix::MakeRectToRect(
        SkRect::MakeSize(anim->size()), SkRect::MakeIWH(128, 128), SkMatrix::kCenter_ScaleToFit));

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
  const sk_sp<SkSurface> fSurface;

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
