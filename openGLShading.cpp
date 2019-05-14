#include "openGLShading.h"

#include <string>

struct TextureInfo {
  uint8_t *buffer;
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

void loadTexture(uint32_t textureID, int width, int height, uint8_t *buffer) {
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
  auto texture1 = textures[0].buffer;
  auto texture2 = textures[1].buffer;
  auto blend_inverse = 1 - target_blend_ratio;
  if (next_invert) {
    for (int i = 0; i < width * height * 3; ++i) {
      outputBuffer[i] = 255 - texture1[i];
      outputBuffer[i+1] = 255 - texture1[i+1];
      outputBuffer[i+2] = 255 - texture1[i+2];
    }
  }
  else {
    for (int i = 0; i < width * height * 3; i = i + 3) {
      int g = i + 1;
      int b = i + 2;
      outputBuffer[i] = (texture1[i] * blend_inverse) + (texture2[i] * target_blend_ratio);
      outputBuffer[g] = (texture1[g] * blend_inverse) + (texture2[g] * target_blend_ratio);
      outputBuffer[b] = (texture1[b] * blend_inverse) + (texture2[b] * target_blend_ratio);
    }
  }
}

void tearDownOpenGL() {
}
