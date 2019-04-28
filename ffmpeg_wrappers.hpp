
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#include <string>
using std::string;

struct TranscodingComponents {
  ~TranscodingComponents();
  AVCodec *codec;
  AVStream *stream;
  AVCodecContext *context;
  AVPacket *packet;
  AVFrame *frame;
};

struct DecodingComponents : TranscodingComponents {
  static DecodingComponents *get_audio_decoder(string file_name);
  int decode_next_audio_frame();
  ~DecodingComponents();
  string file_name;
  AVFormatContext *format_context;
};

struct VideoDecodingComponents : DecodingComponents {
  static VideoDecodingComponents *
  get_video_decoder(string file_name, AVRational expected_framerate);
  ~VideoDecodingComponents();
  int decode_next_video_frame();
  AVFrame *buffered_frame;
  long next_pts;
  long pts_increase_betweem_frames;
};
