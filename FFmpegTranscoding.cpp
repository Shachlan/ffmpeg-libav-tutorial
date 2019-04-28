#include "FFmpegTranscoding.hpp"

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include "libavutil/imgutils.h"
}

TranscodingComponents::~TranscodingComponents() {
  av_frame_free(&frame);
  av_packet_free(&packet);
  avcodec_close(context);
  avcodec_free_context(&context);
}
