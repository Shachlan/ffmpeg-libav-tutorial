#include <unordered_set>

namespace WREOpenGL {
class TexturePool {
public:
  uint32_t get_texture();
  void release_texture(uint32_t name);
  void flush();
  void clear();

private:
  std::unordered_set<uint32_t> available_textures;
  std::unordered_set<uint32_t> used_textures;
};
}  // namespace WREOpenGL
