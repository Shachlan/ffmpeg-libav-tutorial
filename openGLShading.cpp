#include "openGLShading.h"

#include <string>

#if FRONTEND == 1
#include <GLES2/gl2.h>
#include <EGL/egl.h>

#define PIXEL_FORMAT GL_RGBA

#else

#include <OpenGL/gl3.h>

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#define PIXEL_FORMAT GL_RGB

#endif

static std::string vertex_shader_text = "attribute vec2 position;\n"
                                        "attribute vec2 texCoord;\n"
                                        "varying vec2 vTexCoord;\n"
                                        "void main(void) {\n"
                                        "    gl_Position = vec4(position, 0, 1);\n"
                                        "    vTexCoord = texCoord;\n"
                                        "}\n";

static std::string invert_shader_text =
#if FRONTEND == 1
    "precision mediump float;\n"
#endif
    "uniform sampler2D tex;\n"
    "varying vec2 vTexCoord;\n"
    "void main() {\n"
    "    const vec3 kInvert = vec3(1, 1, 1);\n"
    "    gl_FragColor = vec4(kInvert - texture2D(tex, vTexCoord).rgb, 1);\n"
    "}\n";

static std::string blend_shader_text =
#if FRONTEND == 1
    "precision mediump float;\n"
#endif
    "uniform sampler2D tex1;\n"
    "uniform sampler2D tex2;\n"
    "uniform float blendFactor;\n"
    "varying vec2 vTexCoord;\n"
    "\n"
    "void main() {\n"
    "    vec4 color1 = texture2D(tex1, vTexCoord);\n"
    "    vec4 color2 = texture2D(tex2, vTexCoord);\n"
    "    gl_FragColor = (color1 * blendFactor) + (color2 * (1.0 - blendFactor));\n"
    "}\n";

typedef struct
{
    GLuint program;
    GLuint position_buffer;
    GLuint texture_buffer;
} ProgramInfo;

ProgramInfo invert_program;
ProgramInfo blend_program;
#if FRONTEND == 0
GLFWwindow *window;
#endif

static const float position[12] = {
    -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f};

static const float textureCoords[12] = {
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f};

uint32_t
createTexture()
{
    GLuint textureLoc;
    glGenTextures(1, &textureLoc);
    glActiveTexture(GL_TEXTURE0 + textureLoc);

    glBindTexture(GL_TEXTURE_2D, textureLoc);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    return textureLoc;
}

static GLuint build_shader(const GLchar *shader_source, GLenum type)
{
    GLuint shader = glCreateShader(type);
    if (!shader || !glIsShader(shader))
    {
        return 0;
    }
    glShaderSource(shader, 1, &shader_source, 0);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_TRUE)
    {
        return shader;
    }
    GLint logSize = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
    GLchar *errorLog = (GLchar *)calloc(logSize, sizeof(GLchar));
    glGetShaderInfoLog(shader, logSize, NULL, errorLog);
    printf("%s", errorLog);
    free(errorLog);
    return 0;
}

static GLuint position_buffer_setup(GLuint program)
{
    GLuint positionBuf;
    glGenBuffers(1, &positionBuf);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(position), position, GL_STATIC_DRAW);

    GLint loc = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    return positionBuf;
}

static GLuint texture_buffer_setup(GLuint program)
{
    GLuint texturesBuffer;
    glGenBuffers(1, &texturesBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, texturesBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);

    GLint loc = glGetAttribLocation(program, "texCoord");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    return texturesBuffer;
}

static int build_program(std::string *vertex_shader, std::string *fragment_shader)
{
    GLuint v_shader, f_shader;

    if (!((v_shader = build_shader(vertex_shader->c_str(), GL_VERTEX_SHADER)) &&
          (f_shader = build_shader(fragment_shader->c_str(), GL_FRAGMENT_SHADER))))
    {
        return -1;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, v_shader);
    glAttachShader(program, f_shader);
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        exit(1);
    }
    return program;
}

ProgramInfo build_invert_program()
{
    GLuint program = build_program(&vertex_shader_text, &invert_shader_text);
    glUseProgram(program);
    GLuint position_buffer = position_buffer_setup(program);
    GLuint texture_buffer = texture_buffer_setup(program);
    return (ProgramInfo){
        program,
        position_buffer,
        texture_buffer};
}

ProgramInfo build_blend_program(float blend_ratio)
{
    GLuint program = build_program(&vertex_shader_text, &blend_shader_text);
    glUseProgram(program);
    glUniform1f(glGetUniformLocation(program, "blendFactor"), blend_ratio);
    GLuint position_buffer = position_buffer_setup(program);
    GLuint texture_buffer = texture_buffer_setup(program);
    return (ProgramInfo){
        program,
        position_buffer,
        texture_buffer};
}

void setupOpenGL(int width, int height, float blend_ratio, char *canvasName)
{
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
    blend_program = build_blend_program(blend_ratio);
}

void loadTexture(uint32_t textureID, int width, int height, uint8_t *buffer)
{
    glActiveTexture(GL_TEXTURE0 + textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, PIXEL_FORMAT, width, height, 0,
                 PIXEL_FORMAT, GL_UNSIGNED_BYTE, buffer);
}

void invertFrame(uint32_t textureID)
{
    glUseProgram(invert_program.program);
    glUniform1i(glGetUniformLocation(invert_program.program, "tex"), textureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void blendFrames(uint32_t texture1ID, uint32_t texture2ID)
{
    glUseProgram(blend_program.program);
    glUniform1i(glGetUniformLocation(blend_program.program, "tex1"), texture1ID);
    glUniform1i(glGetUniformLocation(blend_program.program, "tex2"), texture2ID);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void getCurrentResults(int width, int height, uint8_t *outputBuffer)
{
    glReadPixels(0, 0, width, height, PIXEL_FORMAT,
                 GL_UNSIGNED_BYTE, outputBuffer);
}

void tearDownOpenGL()
{
    glDeleteProgram(invert_program.program);
    glDeleteBuffers(1, &invert_program.position_buffer);
    glDeleteBuffers(1, &invert_program.texture_buffer);
#if FRONTEND == 0
    glfwDestroyWindow(window);
#endif
}