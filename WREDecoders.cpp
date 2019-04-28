#include "WREDecoders.hpp"

#include "WREFFmpegTranscoding.hpp"
#include "WREVideoFormatConverter.hpp"

long WREDecoder::get_current_timestamp() {
  return internal_decoder->frame->pts;
}

double WREDecoder::get_time_base() {
  return av_q2d(internal_decoder->stream->time_base);
}

double WREDecoder::get_duration() {
  auto stream = internal_decoder->stream;
  AVRational secondary_time_base = stream->time_base;
  return av_q2d(av_mul_q(secondary_time_base, av_make_q(stream->duration, 1)));
}

WREDecoder::~WREDecoder() {
  delete (internal_decoder);
}

WREAudioDecoder::WREAudioDecoder(string file_name) {
  internal_decoder = WREDecodingComponents::get_audio_decoder(file_name);
}

WRETranscodingComponents *WREAudioDecoder::get_transcoding_components() {
  return internal_decoder;
}

int WREAudioDecoder::decode_next_frame() {
  return internal_decoder->decode_next_frame();
}

WREVideoDecoder::WREVideoDecoder(string file_name, double expected_framerate) {
  video_decoder =
      WREVideoDecodingComponents::get_video_decoder(file_name, av_d2q(expected_framerate, 300));
  internal_decoder = video_decoder;
  video_conversion_context =
      WREVideoFormatConverter::create_decoding_conversion_context(video_decoder->context);
}

WREVideoDecoder::~WREVideoDecoder() {
  delete (video_conversion_context);
}

int WREVideoDecoder::get_width() {
  return internal_decoder->context->width;
}

int WREVideoDecoder::get_height() {
  return internal_decoder->context->height;
}

uint8_t *WREVideoDecoder::get_rgb_buffer() {
  return this->video_conversion_context->get_rgb_buffer();
}

int WREVideoDecoder::decode_next_frame() {
  int result = video_decoder->decode_next_frame();
  if (result == 0) {
    this->video_conversion_context->convert_from_frame(this->video_decoder->frame);
  }
  return result;
}
