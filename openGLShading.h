#include <stdint.h>

typedef struct
{
    uint8_t *buffer;
    int width;
    int height;
} TextureInfo;

extern void setupOpenGL(int width, int height, float blend_ratio);

extern void invertFrame(TextureInfo tex);

extern void blendFrames(TextureInfo target, TextureInfo tex1, TextureInfo tex2);

extern void tearDownOpenGL(void);