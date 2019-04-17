#ifndef BUFFERS
#define BUFFERS 1

#include <inttypes.h>

struct VideoFrame {
    int height;
    int width;
    long time_stamp;
    double time_base;
    uint8_t **data;
};

#endif