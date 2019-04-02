#include <stdint.h>

extern void setupOpenGL(int width, int height);

extern void invertFrame(uint8_t *buffer, int width, int height);

extern void tearDownOpenGL(void);