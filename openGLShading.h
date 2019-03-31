#include <libavcodec/avcodec.h>

extern void setupOpenGL(int width, int height);

extern void invertFrame(AVFrame *inFrame, AVFrame *outFrame);

extern void tearDownOpenGL(void);