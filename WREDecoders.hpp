// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

struct WREDecodingComponents;
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
  /// Constructs a decoder of audio streams, that decodes an audio stream into frames.
  /// @param file_name - the path to the file containing the audio stream.
  ///
  /// @param start_from - the point in time, in seconds, from which the audio stream should start.
  /// If \c start_from is larger than the duration of the stream, no frames will be decoded.
  ///
  /// @param duration - duration, in seconds, of the resulting decoded frames. If \c
  /// start_from + \c duration is larger than the duration of the stream, the decoder will decode
  /// until the end of the stream and stop, thus resulting in a shorter than requested duration.
  WREAudioDecoder(string file_name, double start_from = 0, double duration = UINT32_MAX,
                  double speed_ratio = 1);

  /// Returns the transcoding components from which the codec parameters of the decoder can be
  /// received.
  WRETranscodingComponents *get_transcoding_components();
};

/// Decoder of video streams. In addition to decoding the video, the decoder also drops or
/// duplicates decoded frames in order to output them at the requested framerate.
struct WREVideoDecoder : WREDecoder {
  /// Constructs a decoder of audio streams, that decodes an audio stream into frames.
  /// @param file_name - the path to the file containing the audio stream.
  ///
  /// @param expected_framerate - the framerate at which the frame should be decoded. If \c
  /// exptected_framerate will be higher than the framerate of the source stream, then some frames
  /// will be duplicted. If it is smaller, some frames will be dropped.
  ///
  /// @param start_from - the point in time, in seconds, from which the audio stream should start.
  /// If \c start_from is larger than the duration of the stream, no frames will be decoded.
  ///
  /// @param duration - duration, in seconds, of the resulting decoded frames. If \c
  /// start_from + \c duration is larger than the duration of the stream, the decoder will decode
  /// until the end of the stream and stop, thus resulting in a shorter than requested duration.
  ///
  /// @param speed_ration - the ratio between the speed of the original stream and of the decoded
  /// frames.
  ///
  /// These parameters create a mapping between the source stream and resulting frames thus:
  /// If the source stream has X duration with Y frames with average of Z framerate, so that X=Y*Z
  /// and for each source frame has a timestamp of T, then the decoded frames will be mapped to X'=
  /// \c duration, Z' = \c expected_framerate, Y' = ~ X' / (Z' * speed_ratio), T' = (T - \c
  /// start_from) / speed_ratio.
  WREVideoDecoder(string file_name, double expected_framerate = 0, double start_from = 0,
                  double duration = UINT32_MAX, double speed_ratio = 1);
  ~WREVideoDecoder();

  /// Returns a data buffer sized 3 * width * height. The buffer will be populated after
  /// successful calls to \c decode_next_frame.
  uint8_t *get_rgb_buffer();

  /// Returns the width, in pixels, of the decoded video.
  int get_width();

  /// Returns the height, in pixles, of the decoded video.
  int get_height();

  int decode_next_frame() override;

private:
  WREVideoFormatConverter *video_conversion_context;
};
