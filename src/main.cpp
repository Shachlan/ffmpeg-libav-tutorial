#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "WREDecoders.hpp"
#include "WREEncoder.hpp"
#include "openGLShading.hpp"
extern "C" {
#include <libavutil/log.h>
}

#include <OpenGL/gl3.h>

#define FRONTEND 0;

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

    setupOpenGL(width, height);

    double source_time_base = decoder->get_time_base();

    auto primary_texture = get_texture();

    while (decoder->decode_next_frame() >= 0) {
      auto rendererd_text =
          render_lottie(decoder->get_current_timestamp() * decoder->get_time_base());
      loadTexture(primary_texture, decoder->get_width(), decoder->get_height(),
                  decoder->get_rgb_buffer());

      blendFrames(rendererd_text, primary_texture, 0.5);
      getCurrentResults(encoder->get_width(), encoder->get_height(), encoder->get_rgb_buffer());
      encoder->encode_video_frame(source_time_base, decoder->get_current_timestamp());
    }

    double audio_time_base = audio_decoder->get_time_base();
    while (audio_decoder->decode_next_frame() == 0) {
      if (encoder->encode_audio_frame(audio_time_base, audio_decoder->get_current_timestamp()) !=
          0) {
        log_error("audio encoding error");
      }
    }

    encoder->finish_encoding();

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
