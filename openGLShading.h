#include <stdint.h>

typedef struct
{
    uint8_t *buffer;
    int width;
    int height;
} TextureInfo;

extern void setupOpenGL(int width, int height);

extern void invertFrame(TextureInfo tex);

extern void tearDownOpenGL(void);