extern "C" {
#include <libavutil/rational.h>
}

namespace WRETranscoding {

/// Convert \c dbl to a rational number using a constant maximum precision.
inline AVRational wre_double_to_rational(double dbl) {
  return av_d2q(dbl, 300);
}

}  // namespace WRETranscoding
