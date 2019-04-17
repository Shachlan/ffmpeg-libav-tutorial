#include "video_conversion_utilities.hpp"

static struct SwsContext *
conversion_context_from_codec_to_rgb(AVCodecContext *codec) {
  int height = codec->height;
  int width = codec->width;
  return sws_getContext(width, height, codec->pix_fmt, width, height,
                        AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
}

static struct SwsContext *
conversion_context_from_rgb_to_codec(AVCodecContext *codec) {
  int height = codec->height;
  int width = codec->width;
  return sws_getContext(width, height, AV_PIX_FMT_RGB24, width, height,
                        codec->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
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

ConversionContext *create_encoding_conversion_context(AVCodecContext *codec) {
  ConversionContext *context =
      (ConversionContext *)calloc(1, sizeof(ConversionContext));
  context->conversion_context = conversion_context_from_rgb_to_codec(codec);
  context->linesize = linesize_for_codec(codec);
  context->rgb_buffer = rgb_buffer_for_codec(codec);
  return context;
}

ConversionContext *create_decoding_conversion_context(AVCodecContext *codec) {
  ConversionContext *context =
      (ConversionContext *)calloc(1, sizeof(ConversionContext));
  context->conversion_context = conversion_context_from_codec_to_rgb(codec);
  context->linesize = linesize_for_codec(codec);
  context->rgb_buffer = rgb_buffer_for_codec(codec);
  return context;
}

void free_conversion_context(ConversionContext *context) {
  free(context->rgb_buffer[0]);
  free(context->rgb_buffer);
  free(context->linesize);
  sws_freeContext(context->conversion_context);
  free(context);
}

int convert_from_frame(AVFrame *frame, ConversionContext *context) {
  return sws_scale(context->conversion_context,
                   (const uint8_t *const *)frame->data, frame->linesize, 0,
                   frame->height, context->rgb_buffer, context->linesize);
}

int convert_to_frame(AVFrame *frame, ConversionContext *context) {
  return sws_scale(
      context->conversion_context, (const uint8_t *const *)context->rgb_buffer,
      context->linesize, 0, frame->height, frame->data, frame->linesize);
}
