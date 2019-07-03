// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include <unordered_map>

namespace wre_opengl {
class ProgramPool {
public:
  uint32_t get_program(std::string vertex_shader, std::string fragment_shader);
  void release_program(uint32_t program_name);
  void flush();
  void clear();

private:
  void delete_program(uint32_t program);
  std::unordered_map<std::string, uint32_t> description_to_name_mapping;
  std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> name_to_shader_names_mapping;
  std::unordered_map<uint32_t, std::string> name_to_description_mapping;
  std::unordered_map<uint32_t, uint32_t> program_reference_count;
};
}  // namespace wre_opengl
