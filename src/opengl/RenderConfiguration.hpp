#include <vector>

struct RenderConfiguration {
  int vao;
  std::vector<int> textures;
  std::vector<std::pair<string, int>> int_uniforms;
  std::vector<std::pair<string, float>> float_uniforms;
  std::vector<std::pair<string, uint32_t>> attributes;
}