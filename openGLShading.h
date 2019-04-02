#include <stdint.h>

extern void setupOpenGL(int width, int height);

extern void invertFrame(uint8_t *y, uint8_t *u, uint8_t *v, uint8_t *output, int width, int height);

extern void tearDownOpenGL(void);