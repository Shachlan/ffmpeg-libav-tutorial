// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include <core/SkRefCnt.h>
#include <vector>

class SkSurface;
class GrContext;

namespace WRESkiaRendering {
struct Surface;
/// Pool that creates and manages lifetime of Skia surfaces. The pool does not manage the usage of
/// the surfaces, so it is up to the users to ensure that they don't propagate the surfaces they
/// receive to other users. Multiple users using the same surface at the same time might lead to
/// unexpected behavior.
/// All surfaces created by a given instance have the same width/height dimensions. In order to
/// create surfaces with different dimensions, another pool must be created.
class SurfacePool {
public:
  SurfacePool(sk_sp<GrContext> context, int surface_width, int surface_height);
  ~SurfacePool();

  /// Returns an unused Surface. This info might be created by the call, or returned from the
  /// pool of unused surfaces.
  unique_ptr<Surface> get_surface();

  /// Releases the given \c surface into the pool of unused surfaces. A user that calls
  /// this method signals that it no longer uses the given \c surface, and no longer holds
  /// any copies of it. Usage of a surface after releasing it might lead to unexpected behavior.
  void release_surface(Surface &surface);

  /// Disposes of all the currently unused surfaces.
  void flush();

  /// Width of created surfaces.
  const int surface_width;

  /// Height of created surfaces.
  const int surface_height;

private:
  /// Skia context from which surfaces are created.
  const sk_sp<GrContext> context;

  /// Available surfaces that have been created but are currently unused.
  std::vector<unique_ptr<Surface>> available_surfaces;
};
}  // namespace WRESkiaRendering
