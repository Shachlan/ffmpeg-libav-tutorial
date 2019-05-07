
#include <unordered_map>

namespace WREOpenGL {
class ShaderPool {
public:
  int get_program(string vertex_shader, string fragment_shader);
  int release_program(string vertex_shader, string fragment_shader);
  void clear();

private:
  std::unordered_map<string, int> shader_mapping;
  std::unordered_map<string, int> shaders_to_identifier_mapping;
  std::unordered_map<int, string> identifier_to_shaders_mapping;
  std::unordered_map<int, int> program_reference_count;
  std::unordered_map<int, int> shader_reference_count;
};
}  // namespace WREOpenGL
