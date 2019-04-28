#include "Decoders.hpp"

#include "ConversionContext.hpp"
#include "FFmpegTranscoding.hpp"

long Decoder::get_current_timestamp() {
  return internal_decoder->frame->pts;
}

double Decoder::get_time_base() {
  return av_q2d(internal_decoder->stream->time_base);
}

double Decoder::get_duration() {
  auto stream = internal_decoder->stream;
  AVRational secondary_time_base = stream->time_base;
  return av_q2d(av_mul_q(secondary_time_base, av_make_q(stream->duration, 1)));
}

Decoder::~Decoder() {
  delete (internal_decoder);
}

AudioDecoder::AudioDecoder(string file_name) {
  internal_decoder = DecodingComponents::get_audio_decoder(file_name);
}

TranscodingComponents *AudioDecoder::get_transcoding_components() {
  return internal_decoder;
}

int AudioDecoder::decode_next_frame() {
  return internal_decoder->decode_next_audio_frame();
}

VideoDecoder::VideoDecoder(string file_name, double expected_framerate) {
  video_decoder =
      VideoDecodingComponents::get_video_decoder(file_name, av_d2q(expected_framerate, 300));
  internal_decoder = video_decoder;
  video_conversion_context =
      ConversionContext::create_decoding_conversion_context(video_decoder->context);
}

VideoDecoder::~VideoDecoder() {
  delete (video_decoder);
  delete (video_conversion_context);
}

int VideoDecoder::get_width() {
  return internal_decoder->context->width;
}

int VideoDecoder::get_height() {
  return internal_decoder->context->height;
}

uint8_t *VideoDecoder::get_rgb_buffer() {
  return this->video_conversion_context->get_rgb_buffer();
}

int VideoDecoder::decode_next_frame() {
  int result = video_decoder->decode_next_audio_frame();
  if (result == 0) {
    this->video_conversion_context->convert_from_frame(this->video_decoder->frame);
  }
  return result;
}
