// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include <functional>

#include "Transcoding/VideoFormatConverter.hpp"

struct AVFormatContext;

namespace WRETranscoding {

struct TranscodingComponents;
struct VideoFormatConverter;

/// Struct that cleanly wraps audio and video encoding. Video encoding is handled by populating an
/// internal RGB data buffer by using \c write_to_rgb_buffer, while audio encoding is handled
/// directly from the decoder (see note below).
///
/// @important ATM audio encoding is strictly passthrough from a single source, so that the audio
/// source components are passed during construction in order to copy all audio parameters and share
/// all data buffers.
class Encoder {
public:
  /// Opens the file at \c file_name for audio-video encoding. The video will be encoded using \c
  /// video_codec_name codec, with resolution of \c video_width x \c video_height, with \c
  /// video_framerate. \c audio_decoder will be used to initialize the internal audio decoder.
  Encoder(string file_name, string video_codec_name, int video_width, int video_height,
          double video_framerate, const TranscodingComponents *audio_decoder);
  ~Encoder();

  Encoder &operator=(const Encoder &other) = delete;
  Encoder(const Encoder &other) = delete;

  /// Calls the given \c buffer_write function over the internal RGB buffer, sized  3 * width *
  /// height. This will allow the function to access the buffer data and modify it. This data should
  /// be populated with RGB pixel information before calls to \c encode_video_frame. Passing
  /// the pointer outside of \c buffer_write and accessing the buffer from out of the call's scope
  /// might lead to a data race, and is forbidden. Access to the internal buffer is protected only
  /// until \c buffer_write returns, so \c buffer_write cannot start an asynchronous operation
  /// without waiting for all buffer access operations to complete.
  ///
  /// @note TWriteFunc must take \c uint8_t * as a single argument.
  template <class TWriteFunc>
  decltype(auto) write_to_rgb_buffer(TWriteFunc &&buffer_write) {
    return video_conversion_context->write_to_rgb_buffer(buffer_write);
  }

  /// Encodes the information in the internal data buffer as a video frame timestamped as
  /// source_timestamp * source_time_base. Returns 0 if encoding succeeded.
  int encode_video_frame(double source_time_base, long source_timestamp) noexcept;

  /// Encodes the information in the shared internal audio data buffer as an audio frame timestamped
  /// as source_timestamp * source_time_base. Returns 0 if encoding succeeded.
  int encode_audio_frame(double source_time_base, long source_timestamp) noexcept;

  /// Closes the encoder for
  int finish_encoding() noexcept;

  /// Returns the width of the encoded video.
  int get_width() const noexcept;

  /// Returns the height of the encoded video.
  int get_height() const noexcept;

private:
  /// Encoder for video frames.
  std::unique_ptr<TranscodingComponents> video_encoder;

  /// Encoder for audio frames.
  std::unique_ptr<TranscodingComponents> audio_encoder;

  /// Wrapper for the encoded file.
  AVFormatContext *format_context;

  /// Context for converting RGB frames into the format used by \c video_encoder.
  std::unique_ptr<VideoFormatConverter> video_conversion_context;
};

}  // namespace WRETranscoding
