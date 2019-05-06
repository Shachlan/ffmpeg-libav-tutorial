
#include "WREShaderPool.hpp"

#include <fstream>
#include <sstream>

#if FRONTEND == 1
#include <GLES2/gl2.h>
#else
#include <OpenGL/gl3.h>
#endif

using std::pair;
using std::unordered_map;

void WREShaderPool::clear() {
  for (auto &pair : identifier_to_shaders_mapping) {
    glDeleteProgram(pair.first);
  }
  for (auto &pair : shader_mapping) {
    glDeleteShader(pair.second);
  }
  identifier_to_shaders_mapping.clear();
  shaders_to_identifier_mapping.clear();
}

static GLuint build_shader(const GLchar *shader_source, GLenum shader_type) {
  GLuint shader = glCreateShader(shader_type);
  if (!shader || !glIsShader(shader)) {
    return 0;
  }
  glShaderSource(shader, 1, &shader_source, 0);
  glCompileShader(shader);
  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_TRUE) {
    return shader;
  }
  GLint logSize = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
  GLchar *errorLog = (GLchar *)calloc(logSize, sizeof(GLchar));
  glGetShaderInfoLog(shader, logSize, 0, errorLog);
  log_error("%s", errorLog);
  free(errorLog);
  return 0;
}

static string get_shader_text(string shader_file_name) {
  log_debug("reading shader from %s", shader_file_name.c_str());
  std::ifstream stream(shader_file_name);
  std::stringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

int get_shader(string shader_file_name, GLenum shader_type,
               unordered_map<string, int> &shader_mapping) {
  auto search = shader_mapping.find(shader_file_name);
  if (search != shader_mapping.end()) {
    return search->second;
  }

  auto text = get_shader_text(shader_file_name);
  auto shader_identifier = build_shader(text.c_str(), shader_type);
  if (shader_identifier == 0) {
    throw "failed to build shader";
  }
  shader_mapping[shader_file_name] = shader_identifier;
  return shader_identifier;
}

string get_shader_filename(string shader, GLenum shader_type) {
  return shader + (shader_type == GL_VERTEX_SHADER ? ".vsh" : ".fsh");
}

int create_program(int vertex_shader, int fragment_shader) {
  log_debug("creating program %d , %d", vertex_shader, fragment_shader);
  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    exit(1);
  }
  return program;
}

string get_key(string vertex_shader, string fragment_shader) {
  return "vertx:" + vertex_shader + ".fragment:" + fragment_shader;
}

GLint WREShaderPool::get_program(string vertex_shader, string fragment_shader) {
  auto key = get_key(vertex_shader, fragment_shader);
  auto search = this->shaders_to_identifier_mapping.find(key);
  if (search != this->shaders_to_identifier_mapping.end()) {
    return search->second;
  }

  auto v_shader_filename = get_shader_filename(vertex_shader, GL_VERTEX_SHADER);
  auto v_shader_identifier = get_shader(v_shader_filename, GL_VERTEX_SHADER, this->shader_mapping);
  auto f_shader_filename = get_shader_filename(fragment_shader, GL_FRAGMENT_SHADER);
  auto f_shader_identifier =
      get_shader(f_shader_filename, GL_FRAGMENT_SHADER, this->shader_mapping);
  auto program = create_program(v_shader_identifier, f_shader_identifier);

  this->shaders_to_identifier_mapping[key] = program;
  this->identifier_to_shaders_mapping[program] = key;

  return program;
}

// void WREShaderPool::delete_program(int program_identifier) {
//   auto search_identifier = this->identifier_to_shaders_mapping.find(program_identifier);
//   if (search_identifier == this->identifier_to_shaders_mapping.end()) {
//     throw "Could not find wanted program";
//   }

//   auto search_shaders = this->shaders_to_identifier_mapping.find(search_identifier->second);
//   this->shaders_to_identifier_mapping.erase(this->shaders_to_identifier_mapping.begin(),
//                                             search_shaders);
//   this->identifier_to_shaders_mapping.erase(this->identifier_to_shaders_mapping.begin(),
//                                             search_identifier);
//   glDeleteProgram(program_identifier);
// }
