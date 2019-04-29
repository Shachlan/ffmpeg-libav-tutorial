#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

/// Components needed in order to transcode frames using FFmpeg. This struct needs to be initialized
/// by the user, but will dispose of all the components once it is deleted.
struct WRETranscodingComponents {
  ~WRETranscodingComponents();
  AVCodec *codec;
  AVStream *stream;
  AVCodecContext *context;
  AVPacket *packet;
  AVFrame *frame;
};
