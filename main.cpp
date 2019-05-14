#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./openGLShading.h"
#include "Transcoding/Decoders.hpp"
#include "Transcoding/Encoder.hpp"
extern "C" {
#include <libavutil/log.h>
}

using namespace WRETranscoding;

#define FRONTEND 0;

static void invert_single_frame(Encoder *encoder, uint32_t textureID) {
  invertFrame(textureID);
  encoder->write_to_rgb_buffer([&](uint8_t *buffer) {
    getCurrentResults(encoder->get_width(), encoder->get_height(), buffer);
  });
}

static void blend_frames(Encoder *encoder, uint32_t texture1ID, uint32_t texture2ID) {
  blendFrames(texture1ID, texture2ID);
  encoder->write_to_rgb_buffer([&](uint8_t *buffer) {
    getCurrentResults(encoder->get_width(), encoder->get_height(), buffer);
  });
}

int main(int argc, char *argv[]) {
  av_log_set_level(AV_LOG_FATAL);
  double blend_time = 0;
  double invert_time = 0;
  double texture_loading = 0;
  double initial_start = clock();
  log_debug("start");
  int height = 1080;
  int width = 1920;
  auto expected_framerate = 30;
  log_info("setup openGL");
  float blendRatio = strtof(argv[4], NULL);
  setupOpenGL(width, height, blendRatio, NULL);
  uint32_t tex1 = createTexture();
  uint32_t tex2 = createTexture();

  string name = "Xsimd";

  int iterations = atoi(argv[5]);
  for (int k = 0; k < iterations; k++) {
    auto decoder = new VideoDecoder(argv[1], expected_framerate);
    if (decoder == nullptr) {
      log_error("error while preparing input");
      return -1;
    }

    log_info("secondary decoder");
    auto secondary_decoder = new VideoDecoder(argv[2], expected_framerate);
    if (secondary_decoder == nullptr) {
      log_error("error while preparing secondary input");
      return -1;
    }

    log_info("audio decoder");
    auto audio_decoder = new AudioDecoder(argv[1]);
    if (audio_decoder == nullptr) {
      log_error("error while preparing audio input");
      return -1;
    }

    auto encoder = new Encoder(argv[3], "libx264", width, height, expected_framerate,
                               audio_decoder->get_transcoding_components());

    double source_time_base = decoder->get_time_base();
    int response;

    long counted_frames = 0;
    long blended_frames = 0;
    long inverted_frames = 0;
    long audio_frames = 0;
    double start;
    int frames = 0;

    while (decoder->decode_next_frame() >= 0 && secondary_decoder->decode_next_frame() >= 0) {
      start = clock();
      decoder->read_from_rgb_buffer([&](const uint8_t *buffer) {
        loadTexture(tex1, decoder->get_width(), decoder->get_height(), buffer);
      });
      secondary_decoder->read_from_rgb_buffer([&](const uint8_t *buffer) {
        loadTexture(tex2, secondary_decoder->get_width(), secondary_decoder->get_height(), buffer);
      });
      texture_loading += clock() - start;

      start = clock();
      blend_frames(encoder, tex1, tex2);
      blend_time += clock() - start;

      start = clock();
      invert_single_frame(encoder, tex1);
      invert_time += clock() - start;

      encoder->encode_video_frame(source_time_base, decoder->get_current_timestamp());
    }

    double audio_time_base = audio_decoder->get_time_base();
    while (audio_decoder->decode_next_frame() == 0) {
      audio_frames++;
      if (encoder->encode_audio_frame(audio_time_base, audio_decoder->get_current_timestamp()) !=
          0) {
        log_error("audio encoding error");
      }
    }

    encoder->finish_encoding();

    log_info("wrote %lu frames. %lu blended, %lu inverted, %lu audio", counted_frames,
             blended_frames, inverted_frames, audio_frames);

    log_debug("releasing all the resources");

    delete (decoder);
    log_debug("first decoder");
    delete (secondary_decoder);
    log_debug("second decoder");
    delete (audio_decoder);
    log_debug("releasing decoders");
    delete (encoder);
    log_debug("releasing encoder");
  }
  tearDownOpenGL();
  log_debug("teardown opengl");

  blend_time = blend_time / CLOCKS_PER_SEC / iterations;
  invert_time = invert_time / CLOCKS_PER_SEC / iterations;
  texture_loading = texture_loading / CLOCKS_PER_SEC / iterations / 2;
  double total = (clock() - initial_start) / CLOCKS_PER_SEC / iterations;

  printf("\n\n%s: texture loading: %f, blend: %f, invert: %f, total: %f\n\n\n", name.c_str(),
         texture_loading, blend_time, invert_time, total);

  return 0;
}
