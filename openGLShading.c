#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

static const GLchar *v_shader_source =
    "attribute vec2 position;\n"
    "varying vec2 texCoord;\n"
    "void main(void) {\n"
    "  gl_Position = vec4(position, 0, 1);\n"
    "  texCoord = position;\n"
    "}\n";

static const GLchar *f_shader_source =
    "uniform sampler2D tex;\n"
    "varying vec2 texCoord;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(tex, texCoord * 0.5 + 0.5);\n"
    "}\n";

AVFrame *invertFrame(AVFrame *frame) {
    return frame;
}