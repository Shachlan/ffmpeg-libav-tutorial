// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace WRETranscoding {

/// Components needed in order to transcode frames using FFmpeg. This struct needs to be
/// initialized by the user, but will dispose of all the components once it is deleted.
struct TranscodingComponents {
  ~TranscodingComponents();
  AVCodec *codec;
  AVStream *stream;
  AVCodecContext *context;
  AVPacket *packet;
  AVFrame *frame;
  AVRational latest_time_base;
  bool shared_frame;
};

}  // namespace WRETranscoding
