#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./openGLShading.h"
#include "WREDecoders.hpp"
#include "WREEncoder.hpp"
extern "C" {
#include <libavutil/log.h>
}

#define FRONTEND 0;

static void invert_single_frame(WREVideoDecoder *decoder, WREEncoder *encoder, uint32_t textureID) {
  loadTexture(textureID, decoder->get_width(), decoder->get_height(), decoder->get_rgb_buffer());
  invertFrame(textureID);
  getCurrentResults(encoder->get_width(), encoder->get_height(), encoder->get_rgb_buffer());
}

static void blend_frames(WREVideoDecoder *decoder, WREVideoDecoder *secondary_decoder,
                         WREEncoder *encoder, uint32_t texture1ID, uint32_t texture2ID) {
  loadTexture(texture1ID, decoder->get_width(), decoder->get_height(), decoder->get_rgb_buffer());
  loadTexture(texture2ID, secondary_decoder->get_width(), secondary_decoder->get_height(),
              secondary_decoder->get_rgb_buffer());
  blendFrames(texture1ID, texture2ID);
  getCurrentResults(encoder->get_width(), encoder->get_height(), encoder->get_rgb_buffer());
}

int main(int argc, char *argv[]) {
  av_log_set_level(AV_LOG_FATAL);
  log_debug("start");
  int height = 1200;
  int width = 1920;
  auto expected_framerate = 30;

  auto decoder = new WREVideoDecoder(argv[1], expected_framerate, 1, 5, 2);
  if (decoder == nullptr) {
    log_error("error while preparing input");
    return -1;
  }

  log_info("secondary decoder");
  auto secondary_decoder = new WREVideoDecoder(argv[2], expected_framerate);
  if (secondary_decoder == nullptr) {
    log_error("error while preparing secondary input");
    return -1;
  }

  log_info("audio decoder");
  auto audio_decoder = new WREAudioDecoder(argv[1], 1, 4, 2);
  if (audio_decoder == nullptr) {
    log_error("error while preparing audio input");
    return -1;
  }

  WREEncoder *encoder = new WREEncoder(argv[3], "libx264", width, height, expected_framerate,
                                       audio_decoder->get_transcoding_components());

  log_info("next");
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
    if (response < 0) {
      log_error("DECODER: Error while receiving a frame from the secondary decoder: %d", response);
      return response;
    }

    blended_frames++;
    blend_frames(decoder, secondary_decoder, encoder, tex1, tex2);
    encoder->encode_video_frame(source_time_base, decoder->get_current_timestamp());
  }

  double audio_time_base = audio_decoder->get_time_base();
  while (audio_decoder->decode_next_frame() == 0) {
    audio_frames++;
    if (encoder->encode_audio_frame(audio_time_base, audio_decoder->get_current_timestamp()) != 0) {
      log_error("audio encoding error");
    }
  }

  encoder->finish_encoding();

  log_info("wrote %lu frames. %lu blended, %lu inverted, %lu audio", counted_frames, blended_frames,
           inverted_frames, audio_frames);

  log_debug("releasing all the resources");

  delete (decoder);
  log_debug("first decoder");
  delete (secondary_decoder);
  log_debug("second decoder");
  delete (audio_decoder);
  log_debug("releasing decoders");
  // delete (encoder);
  log_debug("releasing encoder");
  tearDownOpenGL();
  log_debug("teardown opengl");
  return 0;
}