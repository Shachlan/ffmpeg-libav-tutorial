// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace WRETranscoding {

/// Returns a \c AVCodecContext pointer, with a specialized deleter.
shared_ptr<AVCodecContext> create_codec_context(shared_ptr<AVCodec> codec);

/// Returns a \c AVFrame pointer, with a specialized deleter.
shared_ptr<AVFrame> create_frame();

/// Returns a \c AVPacket pointer, with a specialized deleter.
shared_ptr<AVPacket> create_packet();

/// Returns a smart pointer wrapping an instance of \c T, with a custom deleter that does nothing.
/// This should be used for classes that don't require any deletion, including no freeing of
/// allocated memory.
template <class T>
inline shared_ptr<T> wrap_with_empty_deleter(T *ptr) {
  return shared_ptr<T>(ptr, [](T *) {});
}

/// Components needed in order to transcode frames using FFmpeg. This struct needs to be
/// initialized by the user, but will dispose of all the components once it is deleted.
struct TranscodingComponents {
  TranscodingComponents() = default;
  TranscodingComponents &operator=(const TranscodingComponents &other) = delete;
  TranscodingComponents(const TranscodingComponents &other) = delete;

  shared_ptr<AVCodec> codec;
  shared_ptr<AVStream> stream;
  shared_ptr<AVCodecContext> context;
  shared_ptr<AVPacket> packet;
  shared_ptr<AVFrame> frame;
  AVRational latest_time_base;
};

}  // namespace WRETranscoding
