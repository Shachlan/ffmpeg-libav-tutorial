// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

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
/// buffer, which can be retrieved using \c get_rgb_buffer.
///
/// @important any given converter can only handle conversion in one way - either from
/// a given frame to an RGB data buffer, or from the data buffer to a given frame. This
/// is determined upon the converter creation, and usage in the other direction will lead
/// to unexpected results.
///
/// This object is threadsafe.
struct VideoFormatConverter {
  ~VideoFormatConverter();

  /// Returns a new converter, from RGB pixel data to the given \c context.
  static VideoFormatConverter *create_encoding_conversion_context(AVCodecContext *context);

  /// Returns a new converter, from the given \c context to RGB pixel data.
  static VideoFormatConverter *create_decoding_conversion_context(AVCodecContext *context);

  /// Converts the data in the given \c frame into the internal RGB buffer. This takes the data in
  /// \c frame, converts its colorspace to the colorspace of the converter and scales the data from
  /// the size of \c frame to the size of the internal buffer.
  void convert_from_frame(AVFrame *frame);

  /// Converts the data in the internal RGB buffer into the given \c frame. This takes the data in
  /// the internal buffer, converts its colorspace to the colorspace of \c frame and scales the data
  /// from the size of the internal buffer to \c frame's size.
  void convert_to_frame(AVFrame *frame);

  /// Calls the given \c buffer_read function over the internal RGB buffer.
  /// This will allow the function to access the buffer da, without modifying it.
  void read_from_rgb_buffer(std::function<void(const uint8_t *)> buffer_read) const;

  /// Calls the given \c buffer_write function over the internal RGB buffer.
  /// This will allow the function to access the buffer data and modify it.
  void write_to_rgb_buffer(std::function<void(uint8_t *)> buffer_write);

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
