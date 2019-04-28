struct TranscodingComponents;
struct AVFormatContext;
struct ConversionContext;

class Encoder {
public:
  Encoder(string file_name, string video_codec_name, int video_width, int video_height,
          double video_framerate, TranscodingComponents *audio_decoder);
  ~Encoder();
  int encode_video_frame(double source_time_base, long source_timestamp);
  int encode_audio_frame(double source_time_base, long source_timestamp);
  int finish_encoding();
  uint8_t *get_rgb_buffer();
  int get_width();
  int get_height();

private:
  TranscodingComponents *video_encoder;
  TranscodingComponents *audio_encoder;
  AVFormatContext *format_context;
  ConversionContext *video_conversion_context;
};