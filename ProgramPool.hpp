
#include <unordered_map>

namespace WREOpenGL {
class ProgramPool {
public:
  int get_program(string vertex_shader, string fragment_shader);
  void release_program(int program_name);
  void flush();
  void clear();

private:
  void delete_program(int program);
  std::unordered_map<string, int> description_to_name_mapping;
  std::unordered_map<int, std::pair<int, int>> name_to_shader_names_mapping;
  std::unordered_map<int, string> name_to_description_mapping;
  std::unordered_map<int, int> program_reference_count;
};
}  // namespace WREOpenGL
