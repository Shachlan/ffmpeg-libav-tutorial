extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include "libavutil/imgutils.h"
}

/// Context used in order to convert data between \c AVFrame objects and an internal RGB data
/// buffer.
///
/// @important any given converter can only handle conversion in one way - either from
/// a given frame to an RGB data buffer, or from the data buffer to a given frame. This
/// is determined upon the converter creation, and usage in the other direction will lead
/// to unexpected results.
struct WREVideoFormatConverter {
  ~WREVideoFormatConverter();

  /// Returns a new converter, from RGB pixel data to the given \c context.
  static WREVideoFormatConverter *create_encoding_conversion_context(AVCodecContext *context);

  /// Returns a new converter, from the given \c context to RGB pixel data.
  static WREVideoFormatConverter *create_decoding_conversion_context(AVCodecContext *context);

  /// Converts the data in the given \c frame into the internal RGB buffer.
  int convert_from_frame(AVFrame *frame);

  /// Converts the data in the internal RGB buffer into the given \c frame.
  int convert_to_frame(AVFrame *frame);

  /// Returns the internal RGB data buffer.
  uint8_t *get_rgb_buffer();

private:
  struct SwsContext *conversion_context;
  int *linesize;
  uint8_t **rgb_buffer;
};
