#include <string>
using std::string;

#include "Buffers.hpp"

struct TranscodingComponents;
struct AVFormatContext;

class Encoder {
public:
  Encoder(string file_name, string video_codec_name, int video_width,
          int video_height, double video_framerate, string audio_codec_name,
          int audio_sample_rate, long audio_bit_rate);
  ~Encoder();
  int encode_video_frame(double source_time_base);
  int encode_audio_frame(double source_time_base);
  int finish_encoding();

  // TODO - make private:
  TranscodingComponents *video_encoder;
  TranscodingComponents *audio_encoder;
  AVFormatContext *format_context;
};