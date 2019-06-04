#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Decoders.hpp"
#include "Encoder.hpp"
#include "openGLShading.hpp"
extern "C" {
#include <libavutil/log.h>
}

#include <OpenGL/gl3.h>

#define FRONTEND 0;

using namespace WRETranscoding;

int main(int argc, char *argv[]) {
  try {
    av_log_set_level(AV_LOG_FATAL);
    log_debug("start");
    int height = 1080;
    int width = 1920;
    auto expected_framerate = 60;

    auto decoder = VideoDecoder(argv[1], expected_framerate, 2, 4, 0.5);

    log_info("secondary decoder");
    auto secondary_decoder = VideoDecoder(argv[2], expected_framerate);

    log_info("audio decoder");
    auto audio_decoder = AudioDecoder(argv[1], 1, 0);

    auto encoder = Encoder(argv[3], "libx264", width, height, expected_framerate,
                           audio_decoder.get_transcoding_components());

    setupOpenGL(width, height, NULL);

    double source_time_base = decoder.get_time_base();

    auto primary_texture = get_texture();

    while (decoder.decode_next_frame() >= 0) {
      auto rendered_text =
          render_text("hello world" +
                      std::to_string(decoder.get_current_timestamp() * decoder.get_time_base()));
      // render_lottie(decoder.get_current_timestamp() * decoder.get_time_base());
      decoder.read_from_rgb_buffer([&](const uint8_t *buffer) {
        loadTexture(primary_texture, decoder.get_width(), decoder.get_height(), buffer);
      });

      blendFrames(rendered_text, primary_texture, 0.5);

      encoder.write_to_rgb_buffer([&](uint8_t *buffer) {
        getCurrentResults(encoder.get_width(), encoder.get_height(), buffer);
      });

      encoder.encode_video_frame(source_time_base, decoder.get_current_timestamp());
    }

    double audio_time_base = audio_decoder.get_time_base();
    while (audio_decoder.decode_next_frame() == 0) {
      if (encoder.encode_audio_frame(audio_time_base, audio_decoder.get_current_timestamp()) != 0) {
        log_error("audio encoding error");
      }
    }

    encoder.finish_encoding();

    log_debug("releasing all the resources");

    // delete (encoder);
    log_debug("releasing encoder");
    tearDownOpenGL();
    log_debug("teardown opengl");
  } catch (std::exception &exception) {
    log_error("Received exception: %s", exception.what());
  }
  return 0;
}
