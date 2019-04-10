#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void setupOpenGL(int width, int height, float blend_ratio, char *canvasName);

extern void invertFrame(uint32_t textureID);

extern void blendFrames(uint32_t texture1ID, uint32_t texture2ID);

extern void tearDownOpenGL(void);

#if FRONTEND == 0
extern uint32_t createTexture(void);
extern void getCurrentResults(int width, int height, uint8_t *outputBuffer);
extern void loadTexture(uint32_t textureID, int width, int height, uint8_t *buffer);
#endif

#ifdef __cplusplus
}
#endif