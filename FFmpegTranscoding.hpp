#ifndef FFMPEG_TRANSCODING
#define FFMPEG_TRANSCODING 1

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

struct TranscodingComponents {
  ~TranscodingComponents();
  AVCodec *codec;
  AVStream *stream;
  AVCodecContext *context;
  AVPacket *packet;
  AVFrame *frame;
};

#endif
