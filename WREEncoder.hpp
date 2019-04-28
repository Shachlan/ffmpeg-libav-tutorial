struct WRETranscodingComponents;
struct AVFormatContext;
struct WREVideoFormatConverter;

/// Struct that cleanly wraps audio and video encoding. Video encoding is handled by populating the
/// RGB data buffer returned from \c get_rgb_buffer, while audio encoding is handled directly from
/// the decoder (see note below).
///
/// @important ATM audio encoding is strictly passthrough from a single source, so that the audio
/// source components are passed during construction in order to copy all audio parameters and share
/// all data buffers.
class WREEncoder {
public:
  /// Opens the file at \c file_name for audio-video encoding. The video will be encoded using \c
  /// video_codec_name codec, with resolution of \c video_width x \c video_height, with \c
  /// video_framerate. \c audio_decoder will be used to initialize the internal audio decoder.
  WREEncoder(string file_name, string video_codec_name, int video_width, int video_height,
             double video_framerate, WRETranscodingComponents *audio_decoder);
  ~WREEncoder();

  /// Returns a data buffer sized 3 * width * height, that will be encoded as an RGB frame.
  uint8_t *get_rgb_buffer();

  /// Encodes the information in the internal data buffer as a video frame timestamped as
  /// source_timestamp * source_time_base;
  int encode_video_frame(double source_time_base, long source_timestamp);

  /// Encodes the information in the shared internal audio data buffer as an audio frame timestamped
  /// as source_timestamp * source_time_base;
  int encode_audio_frame(double source_time_base, long source_timestamp);

  /// Closes the encoder for
  int finish_encoding();

  /// Returns the width of the encoded video.
  int get_width();

  /// Returns the height of the encoded video.
  int get_height();

private:
  WRETranscodingComponents *video_encoder;
  WRETranscodingComponents *audio_encoder;
  AVFormatContext *format_context;
  WREVideoFormatConverter *video_conversion_context;
};