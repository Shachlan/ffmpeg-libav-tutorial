// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "Transcoding/VideoFormatConverter.hpp"

using namespace WRETranscoding;

static struct SwsContext *conversion_context_from_codec_to_rgb(
    std::shared_ptr<AVCodecContext> codec) {
  int height = codec->height;
  int width = codec->width;
  return sws_getContext(width, height, codec->pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BICUBIC,
                        NULL, NULL, NULL);
}

static struct SwsContext *conversion_context_from_rgb_to_codec(
    std::shared_ptr<AVCodecContext> codec) {
  int height = codec->height;
  int width = codec->width;
  return sws_getContext(width, height, AV_PIX_FMT_RGB24, width, height, codec->pix_fmt, SWS_BICUBIC,
                        NULL, NULL, NULL);
}

static int *linesize_for_size(int width) {
  int *linesize = (int *)calloc(4, sizeof(int));
  linesize[0] = 3 * width * sizeof(uint8_t);
  return linesize;
}

static int *linesize_for_codec(std::shared_ptr<AVCodecContext> codec) {
  return linesize_for_size(codec->width);
}

static uint8_t **rgb_buffer_for_size(int width, int height) {
  uint8_t **buffer = (uint8_t **)calloc(1, sizeof(uint8_t *));
  buffer[0] = (uint8_t *)calloc(3 * height * width, sizeof(uint8_t));
  return buffer;
}

uint8_t **rgb_buffer_for_codec(std::shared_ptr<AVCodecContext> codec) {
  return rgb_buffer_for_size(codec->width, codec->height);
}

VideoFormatConverter::VideoFormatConverter(std::shared_ptr<AVCodecContext> codec,
                                           EncodingConverionTag) {
  conversion_context = conversion_context_from_rgb_to_codec(codec);
  linesize = linesize_for_codec(codec);
  rgb_buffer = rgb_buffer_for_codec(codec);
}

VideoFormatConverter::VideoFormatConverter(std::shared_ptr<AVCodecContext> codec,
                                           DecodingConverionTag) {
  conversion_context = conversion_context_from_codec_to_rgb(codec);
  linesize = linesize_for_codec(codec);
  rgb_buffer = rgb_buffer_for_codec(codec);
}

VideoFormatConverter::~VideoFormatConverter() {
  free(rgb_buffer[0]);
  free(rgb_buffer);
  free(linesize);
  sws_freeContext(conversion_context);
}

void VideoFormatConverter::convert_from_frame(std::shared_ptr<AVFrame> frame) noexcept {
  std::unique_lock lock(mutex);
  sws_scale(conversion_context, (const uint8_t *const *)frame->data, frame->linesize, 0,
            frame->height, rgb_buffer, linesize);
}

void VideoFormatConverter::convert_to_frame(std::shared_ptr<AVFrame> frame) noexcept {
  std::shared_lock lock(mutex);
  sws_scale(conversion_context, (const uint8_t *const *)rgb_buffer, linesize, 0, frame->height,
            frame->data, frame->linesize);
}
