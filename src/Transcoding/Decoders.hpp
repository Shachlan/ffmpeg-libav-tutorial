// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include <functional>

#include "Transcoding/VideoFormatConverter.hpp"
namespace WRETranscoding {

struct DecoderImplementation;
struct TranscodingComponents;
struct VideoFormatConverter;

/// Abstract base struct for decoders of single audio or video streams. Implements getters for basic
/// stream information.
struct Decoder {
  Decoder() = default;
  ~Decoder();

  Decoder &operator=(const Decoder &other) = delete;
  Decoder(const Decoder &other) = delete;

  /// Decodes the current frame information from the decoded stream into the internal data buffer,
  /// and advances the stream to the next frame. Returns 0 if decoding succeeded.
  virtual int decode_next_frame() noexcept;

  /// Returns the timestamp of the current frame, in the time base of the decoded stream.
  long get_current_timestamp() const noexcept;

  /// Returns the estimated duration of the decoded stream.
  double get_duration() const noexcept;

  /// Returns the time base of the decoded stream.
  double get_time_base() const noexcept;

protected:
  /// Concrete implementation of the decoding class.
  unique_ptr<DecoderImplementation> decoder_implementation;
};

/// Decoder of audio streams.
struct AudioDecoder : Decoder {
  /// Constructs a decoder of audio streams, that decodes an audio stream into frames.
  ///
  /// @param file_name - the path to the file containing the audio stream.
  ///
  /// @param start_from - the point in time, in seconds, from which the audio stream should start.
  /// If \c start_from is larger than the duration of the stream, no frames will be decoded.
  ///
  /// @param duration - duration, in seconds, of the resulting decoded frames. If \c
  /// start_from + \c duration is larger than the duration of the stream, the decoder will decode
  /// until the end of the stream and stop, thus resulting in a shorter than requested duration.
  AudioDecoder(string file_name, double start_from = 0, double duration = UINT32_MAX);

  /// Returns the transcoding components from which the codec parameters of the decoder can be
  /// received.
  TranscodingComponents *get_transcoding_components() const noexcept;
};

/// Decoder of video streams. In addition to decoding the video, the decoder also drops or
/// duplicates decoded frames in order to output them at the requested framerate.
struct VideoDecoder : Decoder {
  /// Constructs a decoder of audio streams, that decodes an audio stream into frames.
  ///
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
  VideoDecoder(string file_name, double expected_framerate = 0, double start_from = 0,
               double duration = UINT32_MAX, double speed_ratio = 1);

  /// Calls the given \c buffer_read function over the internal RGB buffer, sized 3 * width *
  /// height. This will allow the function to access the buffer da, without modifying it. Passing
  /// the pointer outside of \c buffer_read and accessing the buffer from out of the call's scope
  /// might lead to a data race, and is forbidden. Access to the internal buffer is protected only
  /// until \c buffer_read returns, so \c buffer_read cannot start an asynchronous operation
  /// without waiting for all buffer access operations to complete.
  ///
  /// @note TReadFunc must take \c const \c uint8_t * as a single argument.
  template <class TReadFunc>
  decltype(auto) read_from_rgb_buffer(TReadFunc &&buffer_read) const {
    return video_conversion_context->read_from_rgb_buffer(buffer_read);
  }

  /// Returns the width, in pixels, of the decoded video.
  int get_width() const noexcept;

  /// Returns the height, in pixles, of the decoded video.
  int get_height() const noexcept;

  /// Overrides \c decode_next_frame with the duplication or dropping of frames, according to
  /// \c expected_framerate.
  int decode_next_frame() noexcept override;

private:
  /// Converter between the video's native pixel format and packed RGB format.
  unique_ptr<VideoFormatConverter> video_conversion_context;
};

}  // namespace WRETranscoding
