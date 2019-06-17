#include <stdint.h>

#include "TextureInfo.hpp"

void setupOpenGL(int width, int height, char *canvasName);

void invertFrame(uint32_t textureID);

void blendFrames(uint32_t texture1ID, uint32_t texture2ID, float blend_ratio);

void tearDownOpenGL();

uint32_t render_text(string text, int xCoord, int yCoord, int font_size);

uint32_t render_lottie(double time);

#if FRONTEND == 0
unique_ptr<WREOpenGL::TextureInfo> get_texture(int width, int height);
void release_texture(uint32_t textureID);
void getCurrentResults(int width, int height, uint8_t *outputBuffer);
#endif
