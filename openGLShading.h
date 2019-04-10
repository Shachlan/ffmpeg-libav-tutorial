#include <stdint.h>

extern void setupOpenGL(int width, int height, float blend_ratio, char *canvasName);

extern uint32_t createTexture(void);

extern void loadTexture(uint32_t textureID, int width, int height, uint8_t *buffer);

extern void invertFrame(uint32_t textureID);

extern void blendFrames(uint32_t texture1ID, uint32_t texture2ID);

extern void getCurrentResults(int width, int height, uint8_t *outputBuffer);

extern void tearDownOpenGL(void);