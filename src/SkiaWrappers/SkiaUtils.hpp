#include "SkSurface.h"

sk_sp<SkSurface> create_surface(int width, int height, uint32_t texture_name,
                                sk_sp<GrContext> skia_context);
