// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "Transcoding/TranscodingComponents.hpp"

std::shared_ptr<AVCodecContext> WRETranscoding::create_codec_context(AVCodec *codec) {
  return std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(codec),
                                         [](AVCodecContext *context) {
                                           if (context == nullptr) {
                                             return;
                                           }
                                           avcodec_close(context);
                                           avcodec_free_context(&context);
                                         });
}

std::shared_ptr<AVFrame> WRETranscoding::create_frame() {
  return std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *frame) {
    if (frame == nullptr) {
      return;
    }
    av_frame_unref(frame);
    av_frame_free(&frame);
  });
}

std::shared_ptr<AVPacket> WRETranscoding::create_packet() {
  return std::shared_ptr<AVPacket>(av_packet_alloc(), [](AVPacket *packet) {
    if (packet == nullptr) {
      return;
    }
    av_packet_free(&packet);
  });
}
