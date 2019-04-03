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

GLuint textureLocation;
GLuint program;
GLuint positionBuffers;
GLFWwindow *window;

static const float position[12] = {
    -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f};

static const float textureCoords[12] = {
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f};

#define PIXEL_FORMAT GL_RGB

static char *readFile(char *filename)
{
    FILE *file = fopen(filename, "r");
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *text = calloc(length + 1, sizeof(char));
    if (fread(text, length, 1, file) != 1)
    {
        printf("Failed to read file: %s", filename);
        free(text);
        fclose(file);
        return NULL;
    }
    fclose(file);
    return text;
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
    GLchar *errorLog = calloc(logSize, sizeof(GLchar));
    glGetShaderInfoLog(shader, logSize, NULL, errorLog);
    printf("%s", errorLog);
    free(errorLog);
    return 0;
}

static int build_program()
{
    GLuint v_shader, f_shader;

    char *vertex_shader = readFile("passthrough.vsh");
    char *fragment_shader = readFile("invert.fsh");
    if (!((v_shader = build_shader(vertex_shader, GL_VERTEX_SHADER)) &&
          (f_shader = build_shader(fragment_shader, GL_FRAGMENT_SHADER))))
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
        //printf("Error making program");
        exit(1);
    }
    return program;
}

static GLuint tex_setup(GLuint program)
{
    GLuint textureLoc;
    glGenTextures(1, &textureLoc);
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, textureLoc);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glUniform1i(glGetUniformLocation(program, "tex"), 0);
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

    glGenBuffers(1, &positionBuf);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);

    loc = glGetAttribLocation(program, "texCoord");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    return positionBuf;
}

void setupOpenGL(int width, int height)
{
    //printf("setup\n");
    glfwInit();
    glfwWindowHint(GLFW_VISIBLE, 0);
    window = glfwCreateWindow(width, height, "", NULL, NULL);
    glfwMakeContextCurrent(window);
    glViewport(0, 0, width, height);
    program = build_program();
    glUseProgram(program);
    positionBuffers = vbo_setup(program);
    textureLocation = tex_setup(program);
}

void invertFrame(uint8_t *buffer, int width, int height)
{
    //printf("setup texture\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                 PIXEL_FORMAT, GL_UNSIGNED_BYTE, buffer);
    //printf("draw arrays\n");
    glDrawArrays(GL_TRIANGLES, 0, 6);
    //printf("read pixels\n");
    glReadPixels(0, 0, width, height, PIXEL_FORMAT,
                 GL_UNSIGNED_BYTE, buffer);
}

void tearDownOpenGL()
{
    //printf("teardown\n");
    glDeleteTextures(1, &textureLocation);
    glDeleteProgram(program);
    glDeleteBuffers(1, &positionBuffers);
    glfwDestroyWindow(window);
}