#include "openGLShading.h"

#include <string>

#include <simd/simd.h>

struct TextureInfo {
  const uint8_t *buffer;
  int width; 
  int height;
};

int target_width;
int target_height;
float target_blend_ratio;

TextureInfo textures[2];
int latest_created_texture = -1;

uint32_t createTexture(void) {
  return ++latest_created_texture;
}

bool next_invert;

void setupOpenGL(int width, int height, float blend_ratio, char *canvasName) {
  target_width = width;
  target_height = height;
  target_blend_ratio = blend_ratio;
}

void loadTexture(uint32_t textureID, int width, int height, const uint8_t *buffer) {
  textures[textureID] = (TextureInfo) {
    .width = width,
    .height = height,
    .buffer = buffer
  };
}

void invertFrame(uint32_t textureID) {
  next_invert = true;
}

void blendFrames(uint32_t texture1ID, uint32_t texture2ID) {
  next_invert = false;
}

void getCurrentResults(int width, int height, uint8_t *outputBuffer) {
  auto size = width * height * 3;
  constexpr std::size_t simd_size = 4;
  std::size_t vec_size = size - size % simd_size;
  auto texture1 = textures[0].buffer;
  auto texture2 = textures[1].buffer;
  if (next_invert) {
    for(std::size_t i = 0; i < vec_size; i += simd_size)
    {
      auto rgb = simd_make_uint4(texture1[i], texture1[i + 1], texture1[i + 2], texture1[i + 3]);
      auto res = 255 - rgb;
      outputBuffer[i] = res.x;
      outputBuffer[i + 1] = res.y;
      outputBuffer[i + 2] = res.z;
      outputBuffer[i + 3] = res.w;
    }
  }
  else {
    for(std::size_t i = 0; i < vec_size; i += simd_size)
    {
      auto rgb = simd_make_uint4(texture1[i], texture1[i + 1], texture1[i + 2], texture1[i + 3]);
      auto rgb2 = simd_make_uint4(texture2[i], texture2[i + 1], texture2[i + 2], texture2[i + 3]);
      auto res = (rgb2 + rgb) / 2;
      outputBuffer[i] = res.x;
      outputBuffer[i + 1] = res.y;
      outputBuffer[i + 2] = res.z;
      outputBuffer[i + 3] = res.w;
    }
  }
}

void tearDownOpenGL() {
}
