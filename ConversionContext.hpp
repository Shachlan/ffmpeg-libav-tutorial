#include "./ffmpeg_wrappers.hpp"
extern "C" {
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include "libavutil/imgutils.h"
}
using std::unique_ptr;
using std::weak_ptr;

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
