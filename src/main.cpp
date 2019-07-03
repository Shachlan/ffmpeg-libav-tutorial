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

#include <fstream>
#include <sstream>
#include "OutputModel.hpp"
#include "opengl/RenderingModel.hpp"

#define FRONTEND 0;

using namespace WRETranscoding;

static json read_file(std::string file_name) {
  std::ifstream i(file_name);
  json j;
  i >> j;
  return j;
}

int main(int argc, char *argv[]) {
  auto output_model = OutputModel(read_file("output_model.json"));
  auto render_model = RenderingModel(read_file("render_model.json"));

  try {
    av_log_set_level(AV_LOG_FATAL);
    log_debug("start");
    int height = output_model.height;
    int width = output_model.width;

    auto vector = *(render_model.layers.get());
    auto base_layer = vector.at(0);
    std::shared_ptr<VideoLayer> layer = std::static_pointer_cast<VideoLayer>(base_layer);
    auto decoder =
        VideoDecoder(layer->get_file_name(), layer->get_expected_framerate(),
                     layer->get_start_time(), layer->get_duration(), layer->get_speed_ratio());

    // auto decoder = VideoDecoder(argv[1], expected_framerate, 2, 4, 0.5);

    // log_info("secondary decoder");
    // auto secondary_decoder = VideoDecoder(argv[2], expected_framerate);

    log_info("audio decoder");
    auto audio_decoder = AudioDecoder(argv[1], layer->get_start_time(), layer->get_duration());

    auto encoder =
        Encoder(output_model.source, "libx264", width, height, layer->get_expected_framerate(),
                audio_decoder.get_transcoding_components());

    setupOpenGL(width, height, NULL);

    double source_time_base = decoder.get_time_base();

    auto primary_texture = get_texture();

    while (decoder.decode_next_frame() >= 0) {
      render_text(
          "hello world" + std::to_string(decoder.get_current_timestamp() * decoder.get_time_base()),
          50, 50, 100);
      render_text(
          "you know" + std::to_string(decoder.get_current_timestamp() * decoder.get_time_base()),
          200, 200, 100);
      auto rendered_text = render_text(
          "it works" + std::to_string(decoder.get_current_timestamp() * decoder.get_time_base()),
          300, 300, 100);

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
