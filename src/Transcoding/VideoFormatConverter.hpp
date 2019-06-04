// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#pragma once

#include <functional>
#include <shared_mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include "libavutil/imgutils.h"
}

namespace WRETranscoding {

/// Context used in order to convert data between \c AVFrame objects and an internal RGB data
/// buffer, which can be retrieved using \c read_from_rgb_buffer or modified using
/// \c write_to_rgb_buffer.
///
/// @important any given converter can only handle conversion in one way - either from
/// a given frame to an RGB data buffer, or from the data buffer to a given frame. This
/// is determined upon the converter creation, and usage in the other direction will lead
/// to unexpected results.
///
/// This object is threadsafe.
struct VideoFormatConverter {
  /// Tag for converters used in encoding.
  struct EncodingConverionTag {};

  /// Tag for converters used in decoding.
  struct DecodingConverionTag {};

  /// Construct a new converter, from RGB pixel data to the given \c context.
  VideoFormatConverter(AVCodecContext *context, EncodingConverionTag);

  /// Construct a new converter, from the given \c context to RGB pixel data.
  VideoFormatConverter(AVCodecContext *context, DecodingConverionTag);
  ~VideoFormatConverter();

  VideoFormatConverter &operator=(const VideoFormatConverter &other) = delete;
  VideoFormatConverter(const VideoFormatConverter &other) = delete;

  /// Converts the data in the given \c frame into the internal RGB buffer. This takes the data in
  /// \c frame, converts its colorspace to the colorspace of the converter and scales the data from
  /// the size of \c frame to the size of the internal buffer.
  void convert_from_frame(AVFrame *frame) noexcept;

  /// Converts the data in the internal RGB buffer into the given \c frame. This takes the data in
  /// the internal buffer, converts its colorspace to the colorspace of \c frame and scales the data
  /// from the size of the internal buffer to \c frame's size.
  void convert_to_frame(AVFrame *frame) noexcept;

  /// Calls the given \c buffer_read function over the internal RGB buffer.
  /// This will allow the function to access the buffer da, without modifying it. Passing
  /// the pointer outside of \c buffer_read and accessing the buffer from out of the call's scope
  /// might lead to a data race, and is forbidden. Access to the internal buffer is protected only
  /// until \c buffer_read returns, so \c buffer_read cannot start an asynchronous operation
  /// without waiting for all buffer access operations to complete.
  ///
  /// @note TReadFunc must take \c const \c uint8_t * as a single argument.
  template <class TReadFunc>
  decltype(auto) read_from_rgb_buffer(TReadFunc &&buffer_read) const {
    std::shared_lock lock(mutex);
    return buffer_read(rgb_buffer[0]);
  }

  /// Calls the given \c buffer_write function over the internal RGB buffer.
  /// This will allow the function to access the buffer data and modify it. Passing
  /// the pointer outside of \c buffer_write and accessing the buffer from out of the call's scope
  /// might lead to a data race, and is forbidden. Access to the internal buffer is protected only
  /// until \c buffer_write returns, so \c buffer_write cannot start an asynchronous operation
  /// without waiting for all buffer access operations to complete.
  ///
  /// @note TWriteFunc must take \c uint8_t * as a single argument.
  template <class TWriteFunc>
  decltype(auto) write_to_rgb_buffer(TWriteFunc &&buffer_write) {
    std::unique_lock lock(mutex);
    return buffer_write(rgb_buffer[0]);
  }

private:
  /// Context for conversion between formats.
  struct SwsContext *conversion_context;

  /// Array describing the line size of \c rgb_buffer, for usage in \c conversion_context.
  int *linesize;

  /// Buffer holding the RGB buffer for the \c conversion_context;
  uint8_t **rgb_buffer;

  /// Mutex synchronizing access to the internal RGB buffer.
  mutable std::shared_mutex mutex;
};

}  // namespace WRETranscoding
