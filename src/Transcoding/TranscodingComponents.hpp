// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace WRETranscoding {

/// Returns a \c AVCodecContext pointer, with a specialized deleter.
std::shared_ptr<AVCodecContext> create_codec_context(AVCodec *codec);

/// Returns a \c AVFrame pointer, with a specialized deleter.
std::shared_ptr<AVFrame> create_frame();

/// Returns a \c AVPacket pointer, with a specialized deleter.
std::shared_ptr<AVPacket> create_packet();

/// Components needed in order to transcode frames using FFmpeg. This struct needs to be
/// initialized by the user, but will dispose of all the components once it is deleted.
struct TranscodingComponents {
  TranscodingComponents() = default;
  TranscodingComponents &operator=(const TranscodingComponents &other) = delete;
  TranscodingComponents(const TranscodingComponents &other) = delete;

  AVCodec *codec;
  AVStream *stream;

  std::shared_ptr<AVCodecContext> context;
  std::shared_ptr<AVPacket> packet;
  std::shared_ptr<AVFrame> frame;
  AVRational latest_time_base;
};

}  // namespace WRETranscoding
