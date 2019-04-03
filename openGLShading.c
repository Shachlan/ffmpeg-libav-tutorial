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

typedef struct
{
    GLuint program;
    int texture_count;
    GLuint textures[15];
    GLuint position_buffer;
    GLuint texture_buffer;
} ProgramInfo;

ProgramInfo invert_program;
ProgramInfo blend_program;
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

static int build_program(char *vertex_shader_filename, char *fragment_shader_filename)
{
    GLuint v_shader, f_shader;

    char *vertex_shader = readFile(vertex_shader_filename);
    char *fragment_shader = readFile(fragment_shader_filename);
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

ProgramInfo build_invert_program()
{
    GLuint program = build_program("passthrough.vsh", "invert.fsh");
    glUseProgram(program);
    GLuint position_buffer = position_buffer_setup(program);
    GLuint texture_buffer = texture_buffer_setup(program);
    GLuint texture = tex_setup(program);
    GLuint textures[15];
    textures[0] = texture;
    return (ProgramInfo){
        program,
        1,
        {*textures},
        position_buffer,
        texture_buffer};
}

ProgramInfo build_blend_program(float blend_ratio)
{
    GLuint program = build_program("passthrough.vsh", "blend.fsh");
    glUseProgram(program);
    glUniform1f(glGetUniformLocation(program, "blendFactor"), blend_ratio);
    GLuint position_buffer = position_buffer_setup(program);
    GLuint texture_buffer = texture_buffer_setup(program);
    GLuint textures[15];
    textures[0] = tex_setup(program);
    textures[1] = tex_setup(program);
    return (ProgramInfo){
        program,
        2,
        {*textures},
        position_buffer,
        texture_buffer};
}

void setupOpenGL(int width, int height, float blend_ratio)
{
    //printf("setup\n");
    glfwInit();
    glfwWindowHint(GLFW_VISIBLE, 0);
    window = glfwCreateWindow(width, height, "", NULL, NULL);
    glfwMakeContextCurrent(window);
    glViewport(0, 0, width, height);
    invert_program = build_invert_program();
    blend_program = build_blend_program(blend_ratio);
}

void invertFrame(TextureInfo tex)
{
    glUseProgram(invert_program.program);
    glActiveTexture(GL_TEXTURE0);
    //printf("setup texture\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex.width, tex.height, 0,
                 PIXEL_FORMAT, GL_UNSIGNED_BYTE, tex.buffer);
    //printf("draw arrays\n");
    glDrawArrays(GL_TRIANGLES, 0, 6);
    //printf("read pixels\n");
    glReadPixels(0, 0, tex.width, tex.height, PIXEL_FORMAT,
                 GL_UNSIGNED_BYTE, tex.buffer);
}

extern void blendFrames(TextureInfo target, TextureInfo tex1, TextureInfo tex2)
{
    glUseProgram(invert_program.program);
    glActiveTexture(GL_TEXTURE0);
    //printf("setup texture\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex1.width, tex1.height, 0,
                 PIXEL_FORMAT, GL_UNSIGNED_BYTE, tex1.buffer);

    glActiveTexture(GL_TEXTURE1);
    //printf("setup texture\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex2.width, tex2.height, 0,
                 PIXEL_FORMAT, GL_UNSIGNED_BYTE, tex2.buffer);
    //printf("draw arrays\n");
    glDrawArrays(GL_TRIANGLES, 0, 6);
    //printf("read pixels\n");
    glReadPixels(0, 0, target.width, target.height, PIXEL_FORMAT,
                 GL_UNSIGNED_BYTE, target.buffer);
}

void tearDownOpenGL()
{
    //printf("teardown\n");
    glDeleteTextures(invert_program.texture_count, invert_program.textures);
    glDeleteProgram(invert_program.program);
    glDeleteBuffers(1, &invert_program.position_buffer);
    glDeleteBuffers(1, &invert_program.texture_buffer);
    glfwDestroyWindow(window);
}