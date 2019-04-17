#include "./ffmpeg_wrappers.hpp"


extern "C" {
#include <libavutil/pixdesc.h>
#include "libavutil/imgutils.h"
#include <libswscale/swscale.h>
}

typedef struct _ConversionContext
{
    struct SwsContext *conversion_context;
    int *linesize;
    uint8_t **rgb_buffer;
} ConversionContext;

ConversionContext *create_encoding_conversion_context(AVCodecContext *context);

ConversionContext *create_decoding_conversion_context(AVCodecContext *context);

void free_conversion_context(ConversionContext *);

int convert_from_frame(AVFrame *frame, ConversionContext *context);
int convert_to_frame(AVFrame *frame, ConversionContext *context);
