// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "Transcoding/TranscodingComponents.hpp"

shared_ptr<AVCodecContext> WRETranscoding::create_codec_context(shared_ptr<AVCodec> codec) {
  return shared_ptr<AVCodecContext>(avcodec_alloc_context3(codec.get()),
                                    [](AVCodecContext *context) {
                                      if (context == nullptr) {
                                        return;
                                      }
                                      avcodec_close(context);
                                      avcodec_free_context(&context);
                                    });
}

shared_ptr<AVFrame> WRETranscoding::create_frame() {
  return shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *frame) {
    if (frame == nullptr) {
      return;
    }
    av_frame_unref(frame);
    av_frame_free(&frame);
  });
}

shared_ptr<AVPacket> WRETranscoding::create_packet() {
  return shared_ptr<AVPacket>(av_packet_alloc(), [](AVPacket *packet) {
    if (packet == nullptr) {
      return;
    }
    av_packet_free(&packet);
  });
}
