// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "Transcoding/VideoFormatConverter.hpp"

using namespace WRETranscoding;

static struct SwsContext *conversion_context_from_codec_to_rgb(AVCodecContext *codec) {
  int height = codec->height;
  int width = codec->width;
  return sws_getContext(width, height, codec->pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BICUBIC,
                        NULL, NULL, NULL);
}

static struct SwsContext *conversion_context_from_rgb_to_codec(AVCodecContext *codec) {
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

static int *linesize_for_codec(AVCodecContext *codec) {
  return linesize_for_size(codec->width);
}

static uint8_t **rgb_buffer_for_size(int width, int height) {
  uint8_t **buffer = (uint8_t **)calloc(1, sizeof(uint8_t *));
  buffer[0] = (uint8_t *)calloc(3 * height * width, sizeof(uint8_t));
  return buffer;
}

uint8_t **rgb_buffer_for_codec(AVCodecContext *codec) {
  return rgb_buffer_for_size(codec->width, codec->height);
}

VideoFormatConverter *VideoFormatConverter::create_encoding_conversion_context(
    AVCodecContext *codec) {
  auto context = new VideoFormatConverter();
  context->conversion_context = conversion_context_from_rgb_to_codec(codec);
  context->linesize = linesize_for_codec(codec);
  context->rgb_buffer = rgb_buffer_for_codec(codec);
  return context;
}

VideoFormatConverter *VideoFormatConverter::create_decoding_conversion_context(
    AVCodecContext *codec) {
  auto context = new VideoFormatConverter();
  context->conversion_context = conversion_context_from_codec_to_rgb(codec);
  context->linesize = linesize_for_codec(codec);
  context->rgb_buffer = rgb_buffer_for_codec(codec);
  return context;
}

VideoFormatConverter::~VideoFormatConverter() {
  free(this->rgb_buffer[0]);
  free(this->rgb_buffer);
  free(this->linesize);
  sws_freeContext(this->conversion_context);
}

void VideoFormatConverter::convert_from_frame(AVFrame *frame) {
  std::unique_lock lock(mutex);
  sws_scale(this->conversion_context, (const uint8_t *const *)frame->data, frame->linesize, 0,
            frame->height, this->rgb_buffer, this->linesize);
}

void VideoFormatConverter::convert_to_frame(AVFrame *frame) {
  std::shared_lock lock(mutex);
  sws_scale(this->conversion_context, (const uint8_t *const *)this->rgb_buffer, this->linesize, 0,
            frame->height, frame->data, frame->linesize);
}

void VideoFormatConverter::read_from_rgb_buffer(
    std::function<void(const uint8_t *)> buffer_read) const {
  std::shared_lock lock(mutex);
  buffer_read(rgb_buffer[0]);
}

void VideoFormatConverter::write_to_rgb_buffer(std::function<void(uint8_t *)> buffer_write) {
  std::unique_lock lock(mutex);
  buffer_write(rgb_buffer[0]);
}
