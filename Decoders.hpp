#include "FFmpegTranscoding.hpp"

struct DecodingComponents;
struct VideoDecodingComponents;
struct TranscodingComponents;
struct ConversionContext;

struct Decoder {
  virtual int decode_next_frame() = 0;
  long get_current_timestamp();
  double get_duration();
  double get_time_base();

protected:
  DecodingComponents *internal_decoder;
};

struct AudioDecoder : Decoder {
  AudioDecoder(string file_name);
  ~AudioDecoder();
  int decode_next_frame() override;
  TranscodingComponents *get_transcoding_components();
};

struct VideoDecoder : Decoder {
  VideoDecoder(string file_name, double expected_framerate);
  ~VideoDecoder();
  uint8_t *get_rgb_buffer();
  int get_width();
  int get_height();
  int decode_next_frame() override;

private:
  VideoDecodingComponents *video_decoder;
  ConversionContext *video_conversion_context;
};
