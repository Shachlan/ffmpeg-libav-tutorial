#include <stdint.h>

extern void setupOpenGL(int width, int height, char *canvasName);

extern void invertFrame(uint32_t textureID);

extern void blendFrames(uint32_t texture1ID, uint32_t texture2ID, float blend_ratio);

extern void tearDownOpenGL();

#if FRONTEND == 0
extern uint32_t get_texture();
extern void getCurrentResults(int width, int height, uint8_t *outputBuffer);
extern void loadTexture(uint32_t textureID, int width, int height, uint8_t *buffer);
#endif
