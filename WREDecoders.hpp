struct WREDecodingComponents;
struct WREVideoDecodingComponents;
struct WRETranscodingComponents;
struct WREVideoFormatConverter;

struct WREDecoder {
  virtual ~WREDecoder();
  virtual int decode_next_frame() = 0;
  long get_current_timestamp();
  double get_duration();
  double get_time_base();

protected:
  WREDecodingComponents *internal_decoder;
};

struct WREAudioDecoder : WREDecoder {
  WREAudioDecoder(string file_name);
  int decode_next_frame() override;
  WRETranscodingComponents *get_transcoding_components();
};

struct WREVideoDecoder : WREDecoder {
  WREVideoDecoder(string file_name, double expected_framerate);
  ~WREVideoDecoder();
  uint8_t *get_rgb_buffer();
  int get_width();
  int get_height();
  int decode_next_frame() override;

private:
  WREVideoDecodingComponents *video_decoder;
  WREVideoFormatConverter *video_conversion_context;
};
