// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

struct WREDecodingComponents;
struct WREVideoDecodingComponents;
struct WRETranscodingComponents;
struct WREVideoFormatConverter;

/// Abstract base struct for decoders of single audio or video streams. Implements basic getters for
struct WREDecoder {
  virtual ~WREDecoder();

  /// Decodes the current frame information from the decoded stream into the internal data buffer,
  /// and advances the stream to the next frame. Returns 0 if decoding succeeded.
  virtual int decode_next_frame();

  /// Returns the timestamp of the current frame, in the time base of the decoded stream.
  long get_current_timestamp();

  /// Returns the estimated duration of the decoded stream.
  double get_duration();

  /// Returns the time base of the decoded stream.
  double get_time_base();

protected:
  WREDecodingComponents *internal_decoder;
};

/// Decoder of audio streams.
struct WREAudioDecoder : WREDecoder {
  WREAudioDecoder(string file_name);

  /// Returns the transcoding components from which the codec parameters of the decoder can be
  /// received.
  WRETranscodingComponents *get_transcoding_components();
};

/// Decoder of video streams. In addition to decoding the video, the decoder also drops or
/// duplicates decoded frames in order to output them at the requested framerate.
struct WREVideoDecoder : WREDecoder {
  WREVideoDecoder(string file_name, double expected_framerate);
  ~WREVideoDecoder();

  /// Returns a data buffer sized 3 * width * height. The buffer will be populated after successful
  /// calls to \c decode_next_frame.
  uint8_t *get_rgb_buffer();

  /// Returns the width, in pixels, of the decoded video.
  int get_width();

  /// Returns the height, in pixles, of the decoded video.
  int get_height();

  int decode_next_frame() override;

private:
  WREVideoDecodingComponents *video_decoder;
  WREVideoFormatConverter *video_conversion_context;
};
