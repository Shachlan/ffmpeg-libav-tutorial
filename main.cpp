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

static void invert_single_frame(VideoDecoder *decoder, Encoder *encoder, uint32_t textureID) {
  loadTexture(textureID, decoder->get_width(), decoder->get_height(), decoder->get_rgb_buffer());
  invertFrame(textureID);
  getCurrentResults(encoder->get_width(), encoder->get_height(), encoder->get_rgb_buffer());
}

static void blend_frames(VideoDecoder *decoder, VideoDecoder *secondary_decoder, Encoder *encoder,
                         uint32_t texture1ID, uint32_t texture2ID) {
  loadTexture(texture1ID, decoder->get_width(), decoder->get_height(), decoder->get_rgb_buffer());
  loadTexture(texture2ID, secondary_decoder->get_width(), secondary_decoder->get_height(),
              secondary_decoder->get_rgb_buffer());
  blendFrames(texture1ID, texture2ID);
  getCurrentResults(encoder->get_width(), encoder->get_height(), encoder->get_rgb_buffer());
}

int main(int argc, char *argv[]) {
  logging("start");
  int height = 1200;
  int width = 1920;
  auto expected_framerate = av_q2d(av_make_q(30, 1));

  auto decoder = new VideoDecoder(argv[1], expected_framerate);
  if (decoder == nullptr) {
    logging("error while preparing input");
    return -1;
  }

  logging("secondary decoder");
  auto secondary_decoder = new VideoDecoder(argv[2], expected_framerate);
  if (secondary_decoder == nullptr) {
    logging("error while preparing secondary input");
    return -1;
  }

  logging("audio decoder");
  auto audio_decoder = new AudioDecoder(argv[1]);
  if (audio_decoder == nullptr) {
    logging("error while preparing audio input");
    return -1;
  }

  Encoder *encoder = new Encoder(argv[3], "libx264", width, height, expected_framerate,
                                 audio_decoder->get_transcoding_components());

  logging("next");
  float blendRatio = strtof(argv[4], NULL);
  printf("blend ratio: %f\n", blendRatio);
  int wait = atoi(argv[5]);
  int length = atoi(argv[6]);

  setupOpenGL(width, height, blendRatio, NULL);
  uint32_t tex1 = createTexture();
  uint32_t tex2 = createTexture();

  double source_time_base = decoder->get_time_base();
  double duration_in_seconds = secondary_decoder->get_duration();
  double maxTime = (length > duration_in_seconds ? duration_in_seconds : length) + wait;
  printf("duration: %lf, max time: %lf, wait: %d\n", duration_in_seconds, maxTime, wait);
  int response;

  long counted_frames = 0;
  long blended_frames = 0;
  long inverted_frames = 0;
  long audio_frames = 0;
  while (decoder->decode_next_frame() >= 0) {
    counted_frames++;

    auto point_in_time = source_time_base * decoder->get_current_timestamp();
    // logging("point in time %f", point_in_time);
    if (point_in_time < wait || point_in_time > maxTime) {
      inverted_frames++;
      invert_single_frame(decoder, encoder, tex1);
      encoder->encode_video_frame(source_time_base, decoder->get_current_timestamp());
      // av_frame_unref(inputFrame);
      continue;
    }

    response = secondary_decoder->decode_next_frame();
    if (response < 0 && response != AVERROR_EOF) {
      logging(
          "DECODER: Error while receiving a frame from the secondary "
          "decoder: %d %s",
          response, av_err2str(response));
      return response;
    } else if (response == AVERROR_EOF) {
      inverted_frames++;
      invert_single_frame(decoder, encoder, tex1);
      encoder->encode_video_frame(source_time_base, decoder->get_current_timestamp());
      continue;
    }

    blended_frames++;
    blend_frames(decoder, secondary_decoder, encoder, tex1, tex2);
    encoder->encode_video_frame(source_time_base, decoder->get_current_timestamp());
  }

  double audio_time_base = audio_decoder->get_time_base();
  while (audio_decoder->decode_next_frame() == 0) {
    audio_frames++;
    if (encoder->encode_audio_frame(audio_time_base, audio_decoder->get_current_timestamp()) != 0) {
      logging("audio encoding error");
    }
  }

  encoder->finish_encoding();

  logging("wrote %lu frames. %lu blended, %lu inverted, %lu audio", counted_frames, blended_frames,
          inverted_frames, audio_frames);

  logging("releasing all the resources");

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