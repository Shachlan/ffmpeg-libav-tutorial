#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
extern "C" {
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include "./rationalExtensions.h"
}
#include <inttypes.h>
#include <string.h>
#include "./ConversionContext.hpp"
#include "./openGLShading.h"
#include "Decoders.hpp"
#include "Encoder.hpp"

#define FRONTEND 0;

static void invert_single_frame(AVFrame *inputFrame, Encoder *encoder, uint32_t textureID,
                                ConversionContext *decoding_conversion) {
  // printf("start convert frame\n");
  decoding_conversion->convert_from_frame(inputFrame);
  // printf("invert frame\n");
  loadTexture(textureID, inputFrame->width, inputFrame->height,
              decoding_conversion->get_rgb_buffer());
  invertFrame(textureID);
  getCurrentResults(encoder->get_width(), encoder->get_height(), encoder->get_rgb_buffer());
}

static void blend_frames(AVFrame *inputFrame, AVFrame *secondary_input_frame, Encoder *encoder,
                         uint32_t texture1ID, uint32_t texture2ID,
                         ConversionContext *decoding_conversion,
                         ConversionContext *secondary_decoding_conversion) {
  decoding_conversion->convert_from_frame(inputFrame);
  decoding_conversion->convert_from_frame(secondary_input_frame);
  // printf("blend frames\n");
  loadTexture(texture1ID, inputFrame->width, inputFrame->height,
              decoding_conversion->get_rgb_buffer());
  loadTexture(texture2ID, secondary_input_frame->width, secondary_input_frame->height,
              secondary_decoding_conversion->get_rgb_buffer());
  blendFrames(texture1ID, texture2ID);
  getCurrentResults(encoder->get_width(), encoder->get_height(), encoder->get_rgb_buffer());
}

int main(int argc, char *argv[]) {
  logging("start");
  VideoDecodingComponents *decoder;
  DecodingComponents *audio_decoder;
  VideoDecodingComponents *secondary_decoder;
  int height = 1200;
  int width = 1920;
  AVRational expected_framerate = av_make_q(30, 1);

  logging("video decoder");
  decoder = VideoDecodingComponents::get_video_decoder(argv[1], expected_framerate);
  if (decoder == nullptr) {
    logging("error while preparing input");
    return -1;
  }

  logging("secondary decoder");
  secondary_decoder = VideoDecodingComponents::get_video_decoder(argv[2], expected_framerate);
  if (secondary_decoder == nullptr) {
    logging("error while preparing secondary input");
    return -1;
  }

  logging("audio decoder");
  audio_decoder = DecodingComponents::get_audio_decoder(argv[1]);
  if (audio_decoder == nullptr) {
    logging("error while preparing audio input");
    return -1;
  }

  Encoder *encoder = new Encoder(argv[3], "libx264", width, height, 30, audio_decoder);

  logging("next");
  float blendRatio = strtof(argv[4], NULL);
  printf("blend ratio: %f\n", blendRatio);
  int wait = atoi(argv[5]);
  int length = atoi(argv[6]);

  AVCodecContext *secondary_video_context = secondary_decoder->context;
  setupOpenGL(width, height, blendRatio, NULL);
  uint32_t tex1 = createTexture();
  uint32_t tex2 = createTexture();
  ConversionContext *input_conversion_context =
      ConversionContext::create_decoding_conversion_context(decoder->context);
  ConversionContext *secondary_input_conversion_context =
      ConversionContext::create_decoding_conversion_context(secondary_video_context);

  AVStream *secondary_video_stream = secondary_decoder->stream;

  AVRational time_base = decoder->stream->time_base;
  AVRational secondary_time_base = secondary_video_stream->time_base;
  double duration_in_seconds =
      av_q2d(multiply_by_int(secondary_time_base, secondary_video_stream->duration));
  double maxTime = (length > duration_in_seconds ? duration_in_seconds : length) + wait;
  printf("duration: %lf, max time: %lf, wait: %d\n", duration_in_seconds, maxTime, wait);
  int response;

  long counted_frames = 0;
  long blended_frames = 0;
  long inverted_frames = 0;
  long audio_frames = 0;
  while (decoder->decode_next_video_frame() >= 0) {
    counted_frames++;

    double source_time_base = av_q2d(decoder->stream->time_base);
    AVRational current_pos = multiply_by_int(time_base, decoder->packet->pts);
    double point_in_time = av_q2d(current_pos);
    // logging("point in time %f", point_in_time);
    if (point_in_time < wait || point_in_time > maxTime) {
      inverted_frames++;
      invert_single_frame(decoder->frame, encoder, tex1, input_conversion_context);
      encoder->encode_video_frame(source_time_base, decoder->frame->pts);
      // av_frame_unref(inputFrame);
      continue;
    }

    response = secondary_decoder->decode_next_video_frame();
    if (response < 0 && response != AVERROR_EOF) {
      logging(
          "DECODER: Error while receiving a frame from the secondary "
          "decoder: %d %s",
          response, av_err2str(response));
      return response;
    } else if (response == AVERROR_EOF) {
      inverted_frames++;
      invert_single_frame(decoder->frame, encoder, tex1, input_conversion_context);
      encoder->encode_video_frame(source_time_base, decoder->frame->pts);
      continue;
    }

    blended_frames++;
    blend_frames(decoder->frame, secondary_decoder->frame, encoder, tex1, tex2,
                 input_conversion_context, secondary_input_conversion_context);
    encoder->encode_video_frame(source_time_base, decoder->frame->pts);
  }

  double audio_time_base = av_q2d(audio_decoder->stream->time_base);
  while (audio_decoder->decode_next_audio_frame() == 0) {
    audio_frames++;
    if (encoder->encode_audio_frame(audio_time_base, audio_decoder->frame->pts) != 0) {
      logging("audio encoding error");
    }
  }

  encoder->finish_encoding();

  logging("wrote %lu frames. %lu blended, %lu inverted, %lu audio", counted_frames, blended_frames,
          inverted_frames, audio_frames);

  logging("releasing all the resources");

  delete (input_conversion_context);
  delete (secondary_input_conversion_context);
  logging("releasing conversions");
  delete (decoder);
  delete (secondary_decoder);
  delete (audio_decoder);
  logging("releasing decoders");
  // delete (encoder);
  logging("releasing encoder");
  tearDownOpenGL();
  logging("teardown opengl");
  return 0;
}