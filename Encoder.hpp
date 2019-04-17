#include <string>
using std::string;

#include "Buffers.hpp"

struct TranscodingComponents;
struct AVFormatContext;

class Encoder {
public:
  Encoder(string file_name, string video_codec_name, int width, int height,
          double framerate);
  ~Encoder();
  int encode_video_frame(double source_time_base);
  int encode_audio_frame(double source_time_base);
  int finish_encoding();

  // TODO - make private:
  TranscodingComponents *video_encoder;
  TranscodingComponents *audio_encoder;
  AVFormatContext *format_context;
};