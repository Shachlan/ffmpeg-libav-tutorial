// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#if FRONTEND == 1
#include <GLES2/gl2.h>
#else
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif

#include "opengl/GLException.hpp"
