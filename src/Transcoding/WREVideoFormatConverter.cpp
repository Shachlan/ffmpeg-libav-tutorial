#include "WREVideoFormatConverter.hpp"

#pragma region resources - allocation

static struct SwsContext *conversion_context_from_codec_to_rgb(AVCodecContext *codec) {
  int height = codec->height;
  int width = codec->width;
  return sws_getContext(width, height, codec->pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BICUBIC,
                        NULL, NULL, NULL);
}

static struct SwsContext *conversion_context_from_rgb_to_codec(AVCodecContext *codec) {
  int height = codec->height;
  int width = codec->width;
  return sws_getContext(width, height, AV_PIX_FMT_RGBA, width, height, codec->pix_fmt, SWS_BICUBIC,
                        NULL, NULL, NULL);
}

static int *linesize_for_size(int width, int multiplier) {
  int *linesize = (int *)calloc(4, sizeof(int));
  linesize[0] = multiplier * width * sizeof(uint8_t);
  return linesize;
}

static int *linesize_for_codec(AVCodecContext *codec, int multiplier) {
  return linesize_for_size(codec->width, multiplier);
}

static uint8_t **rgb_buffer_for_size(int width, int height, int multiplier) {
  uint8_t **buffer = (uint8_t **)calloc(1, sizeof(uint8_t *));
  buffer[0] = (uint8_t *)calloc(multiplier * height * width, sizeof(uint8_t));
  return buffer;
}

uint8_t **rgb_buffer_for_codec(AVCodecContext *codec, int multiplier) {
  return rgb_buffer_for_size(codec->width, codec->height, multiplier);
}

#pragma endregion
#pragma region constructors destructors

WREVideoFormatConverter *WREVideoFormatConverter::create_encoding_conversion_context(
    AVCodecContext *codec) {
  auto context = new WREVideoFormatConverter();
  context->conversion_context = conversion_context_from_rgb_to_codec(codec);
  context->linesize = linesize_for_codec(codec, 4);
  context->rgb_buffer = rgb_buffer_for_codec(codec, 4);
  return context;
}

WREVideoFormatConverter *WREVideoFormatConverter::create_decoding_conversion_context(
    AVCodecContext *codec) {
  auto context = new WREVideoFormatConverter();
  context->conversion_context = conversion_context_from_codec_to_rgb(codec);
  context->linesize = linesize_for_codec(codec, 3);
  context->rgb_buffer = rgb_buffer_for_codec(codec, 3);
  return context;
}

WREVideoFormatConverter::~WREVideoFormatConverter() {
  free(this->rgb_buffer[0]);
  free(this->rgb_buffer);
  free(this->linesize);
  sws_freeContext(this->conversion_context);
}

#pragma endregion
#pragma region conversions

int WREVideoFormatConverter::convert_from_frame(AVFrame *frame) {
  return sws_scale(this->conversion_context, (const uint8_t *const *)frame->data, frame->linesize,
                   0, frame->height, this->rgb_buffer, this->linesize);
}

int WREVideoFormatConverter::convert_to_frame(AVFrame *frame) {
  return sws_scale(this->conversion_context, (const uint8_t *const *)this->rgb_buffer,
                   this->linesize, 0, frame->height, frame->data, frame->linesize);
}

#pragma endregion
#pragma region getters

uint8_t *WREVideoFormatConverter::get_rgb_buffer() {
  return this->rgb_buffer[0];
}

#pragma endregion
