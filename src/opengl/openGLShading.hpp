#include <stdint.h>

void setupOpenGL(int width, int height, char *canvasName);

void invertFrame(uint32_t textureID);

void blendFrames(uint32_t texture1ID, uint32_t texture2ID, float blend_ratio);

void tearDownOpenGL();

#if FRONTEND == 0
uint32_t get_texture();
void getCurrentResults(int width, int height, uint8_t *outputBuffer);
void loadTexture(uint32_t textureID, int width, int height, const uint8_t *buffer);
#endif
