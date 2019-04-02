#include "openGLShading.h"

#include <libavutil/pixdesc.h>

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

GLuint yTextureLocation;
GLuint uTextureLocation;
GLuint vTextureLocation;
GLuint program;
GLuint positionBuffers;
GLFWwindow *window;

static const float position[12] = {
    -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f};

#define PIXEL_FORMAT GL_RGB

static const GLchar *v_shader_source =
    "attribute vec2 position;\n"
    "varying vec2 texCoord;\n"
    "void main(void) {\n"
    "  gl_Position = vec4(position, 0, 1);\n"
    "  texCoord = position;\n"
    "}\n";

static const GLchar *f_shader_source =
    "varying vec2 texCoord;\n"
    "uniform sampler2D y_texture;\n"
    "uniform sampler2D u_texture;\n"
    "uniform sampler2D v_texture;\n"
    "const vec3 kInvert = vec3(1)\n"
    "void main() {\n"
    "float r, g, b, y, u, v;\n"
    "y = texture2D(y_texture, texCoord).r;\n "
    "u = texture2D(u_texture, texCoord).r;\n "
    "v = texture2D(v_texture, texCoord).r;\n "
    "y = 1.1643 * (y - 0.0625);\n"
    "u = u - 0.5;\n"
    "v = v - 0.5;\n"
    "r = y + 1.5958 * v;\n"
    "g = y - 0.39173 * u - 0.81290 * v;\n"
    "b = y + 2.017 * u;\n"
    "vec3 invertedColor = kInvert - vec3(r,g,b);"
    "gl_FragColor = vec4(invertedColor, 1);\n"
    "}\n";

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
    return status == GL_TRUE ? shader : 0;
}

static int build_program()
{
    GLuint v_shader, f_shader;

    if (!((v_shader = build_shader(v_shader_source, GL_VERTEX_SHADER)) &&
          (f_shader = build_shader(f_shader_source, GL_FRAGMENT_SHADER))))
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
        printf("Error making program");
        exit(1);
    }
    return program;
}

static GLuint tex_setup(GLuint program, GLchar *name, GLint number, GLenum texture)
{
    GLuint textureLoc;
    glGenTextures(1, &textureLoc);
    glActiveTexture(texture);

    glBindTexture(GL_TEXTURE_2D, textureLoc);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glUniform1i(glGetUniformLocation(program, name), number);
    return textureLoc;
}

static GLuint vbo_setup(GLuint programm)
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

void setupOpenGL(int width, int height)
{
    printf("setup\n");
    glfwInit();
    glfwWindowHint(GLFW_VISIBLE, 0);
    window = glfwCreateWindow(width, height, "", NULL, NULL);
    glfwMakeContextCurrent(window);
    glViewport(0, 0, width, height);
    program = build_program();
    glUseProgram(program);
    positionBuffers = vbo_setup(program);
    yTextureLocation = tex_setup(program, "y_texture", 0, GL_TEXTURE0);
    uTextureLocation = tex_setup(program, "u_texture", 1, GL_TEXTURE1);
    vTextureLocation = tex_setup(program, "v_texture", 2, GL_TEXTURE2);
}

void invertFrame(uint8_t *y, uint8_t *u, uint8_t *v, uint8_t *output, int width, int height)
{

    printf("setup texture\n");
    glActiveTexture(GL_TEXTURE0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R, width, height, 0,
                 PIXEL_FORMAT, GL_UNSIGNED_BYTE, y);
    glActiveTexture(GL_TEXTURE1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R, width / 2, height, 0,
                 PIXEL_FORMAT, GL_UNSIGNED_BYTE, u);
    glActiveTexture(GL_TEXTURE2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R, width / 2, height, 0,
                 PIXEL_FORMAT, GL_UNSIGNED_BYTE, v);
    printf("draw arrays\n");
    glDrawArrays(GL_TRIANGLES, 0, 6);
    printf("read pixels\n");
    glReadPixels(0, 0, width, height, PIXEL_FORMAT, GL_UNSIGNED_BYTE, output);
}

void tearDownOpenGL()
{
    printf("teardown\n");
    glDeleteTextures(1, &yTextureLocation);
    glDeleteTextures(1, &uTextureLocation);
    glDeleteTextures(1, &vTextureLocation);
    glDeleteProgram(program);
    glDeleteBuffers(1, &positionBuffers);
    glfwDestroyWindow(window);
}