
#include "ProgramPool.hpp"

#include <fstream>
#include <sstream>
#include <vector>

#if FRONTEND == 1
#include <GLES2/gl2.h>
#else
#include <OpenGL/gl3.h>
#endif

using namespace WREOpenGL;

void ProgramPool::delete_program(GLuint program_name) {
  auto shaders = this->name_to_shader_names_mapping[program_name];
  auto description = this->name_to_description_mapping[program_name];

  glDeleteProgram(program_name);
  glDeleteShader(shaders.first);
  glDeleteShader(shaders.second);
}

void ProgramPool::flush() {
  std::vector<GLuint> names_to_remove;
  for (auto &pair : program_reference_count) {
    if (pair.second > 0) {
      continue;
    }
    delete_program(pair.first);
    names_to_remove.insert(names_to_remove.end(), pair.first);
  }

  for (auto &name : names_to_remove) {
    auto description = this->name_to_description_mapping[name];
    this->description_to_name_mapping.erase(description);
    this->name_to_shader_names_mapping.erase(name);
    this->name_to_description_mapping.erase(name);
    this->program_reference_count.erase(name);
  }
}

void ProgramPool::clear() {
  for (auto &pair : name_to_description_mapping) {
    delete_program(pair.first);
  }

  this->description_to_name_mapping.clear();
  this->name_to_shader_names_mapping.clear();
  this->name_to_description_mapping.clear();
  this->program_reference_count.clear();
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

int build_shader(string shader_file_name, GLenum shader_type) {
  auto text = get_shader_text(shader_file_name);
  auto shader_name = build_shader(text.c_str(), shader_type);
  if (shader_name == 0) {
    throw "failed to build shader";
  }

  return shader_name;
}

string get_shader_filename(string shader, GLenum shader_type) {
  return shader + (shader_type == GL_VERTEX_SHADER ? ".vsh" : ".fsh");
}

GLuint create_program(GLuint vertex_shader, GLuint fragment_shader) {
  log_debug("creating program %d , %d", vertex_shader, fragment_shader);
  GLuint program = glCreateProgram();
  log_debug("created program");
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  log_debug("attached shaders");
  glLinkProgram(program);
  log_debug("linked program");

  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  log_debug("got status &d", status);
  if (status != GL_TRUE) {
    exit(1);
  }
  log_debug("created program %d", program);
  return program;
}

string get_key(string vertex_shader, string fragment_shader) {
  return "vertx:" + vertex_shader + ".fragment:" + fragment_shader;
}

GLuint ProgramPool::get_program(string vertex_shader, string fragment_shader) {
  auto key = get_key(vertex_shader, fragment_shader);
  auto search = this->description_to_name_mapping.find(key);
  if (search != this->description_to_name_mapping.end()) {
    this->program_reference_count[search->second]++;
    return search->second;
  }

  auto v_shader_filename = get_shader_filename(vertex_shader, GL_VERTEX_SHADER);
  auto v_shader_name = build_shader(v_shader_filename, GL_VERTEX_SHADER);
  auto f_shader_filename = get_shader_filename(fragment_shader, GL_FRAGMENT_SHADER);
  auto f_shader_name = build_shader(f_shader_filename, GL_FRAGMENT_SHADER);
  auto program = create_program(v_shader_name, f_shader_name);

  this->description_to_name_mapping[key] = program;
  this->name_to_shader_names_mapping[program] = std::pair<int, int>{v_shader_name, f_shader_name};
  this->name_to_description_mapping[program] = key;
  this->program_reference_count[program] = 1;

  return program;
}

void ProgramPool::release_program(GLuint program_name) {
  program_reference_count[program_name]--;
}
