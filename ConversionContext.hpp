extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include "libavutil/imgutils.h"
}

struct ConversionContext {
  static ConversionContext *create_encoding_conversion_context(AVCodecContext *context);
  static ConversionContext *create_decoding_conversion_context(AVCodecContext *context);
  int convert_from_frame(AVFrame *frame);
  int convert_to_frame(AVFrame *frame);
  uint8_t *get_rgb_buffer();
  ~ConversionContext();

private:
  struct SwsContext *conversion_context;
  int *linesize;
  uint8_t **rgb_buffer;
};
