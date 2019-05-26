#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./openGLShading.hpp"
#include "WREDecoders.hpp"
#include "WREEncoder.hpp"
extern "C" {
#include <libavutil/log.h>
}

#include <OpenGL/gl3.h>

#define FRONTEND 0;

// static void invert_single_frame(WREVideoDecoder *decoder, WREEncoder *encoder, uint32_t
// textureID) {
//   loadTexture(textureID, decoder->get_width(), decoder->get_height(), decoder->get_rgb_buffer());
//   invertFrame(textureID);
//   getCurrentResults(encoder->get_width(), encoder->get_height(), encoder->get_rgb_buffer());
// }

// static void blend_frames(WREVideoDecoder *decoder, WREVideoDecoder *secondary_decoder,
//                          WREEncoder *encoder, uint32_t texture1ID, uint32_t texture2ID,
//                          float blend_ratio) {
//   loadTexture(texture1ID, decoder->get_width(), decoder->get_height(),
//   decoder->get_rgb_buffer()); loadTexture(texture2ID, secondary_decoder->get_width(),
//   secondary_decoder->get_height(),
//               secondary_decoder->get_rgb_buffer());
//   blendFrames(texture1ID, texture2ID, blend_ratio);
//   getCurrentResults(encoder->get_width(), encoder->get_height(), encoder->get_rgb_buffer());
// }

int main(int argc, char *argv[]) {
  try {
    av_log_set_level(AV_LOG_FATAL);
    log_debug("start");
    int height = 1080;
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
    int wait = atoi(argv[5]);
    int length = atoi(argv[6]);

    setupOpenGL(width, height, NULL);

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
      glClearColor(0, 0, 0, 0);
      glClear(GL_COLOR_BUFFER_BIT);
      auto rendererd_text =
          render_text("Inverting " + std::to_string(decoder->get_current_timestamp()));
      auto loaded_texture =
          loadTexture(decoder->get_width(), decoder->get_height(), decoder->get_rgb_buffer());
      blendFrames(rendererd_text, loaded_texture, 0.5);
      getCurrentResults(encoder->get_width(), encoder->get_height(), encoder->get_rgb_buffer());
      release_texture(loaded_texture);
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
    // delete (encoder);
    log_debug("releasing encoder");
    tearDownOpenGL();
    log_debug("teardown opengl");
  } catch (std::exception &exception) {
    log_error("Received exception: %s", exception.what());
  }
  return 0;
}
