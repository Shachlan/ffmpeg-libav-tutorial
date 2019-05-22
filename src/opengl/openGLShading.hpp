#include <stdint.h>

void setupOpenGL(int width, int height, char *canvasName);

void invertFrame(uint32_t textureID);

void blendFrames(uint32_t texture1ID, uint32_t texture2ID, float blend_ratio);

void tearDownOpenGL();

uint32_t render_text(string text);

#if FRONTEND == 0
uint32_t get_texture();
void release_texture(uint32_t textureID);
void getCurrentResults(int width, int height, uint8_t *outputBuffer);
uint32_t loadTexture(int width, int height, const uint8_t *buffer);
#endif
