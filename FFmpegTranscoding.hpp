#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

/// Components needed in order to transcode frames using FFmpeg. This struct needs to be initialized
/// by the user, but will dispose of all the components once it is deleted.
struct WRETranscodingComponents {
  ~WRETranscodingComponents();
  AVCodec *codec;
  AVStream *stream;
  AVCodecContext *context;
  AVPacket *packet;
  AVFrame *frame;
};

/// Components needed in order to decode media from a given file using FFmpeg. This struct cleanly
/// wraps the frame decoding process.
struct WREDecodingComponents : WRETranscodingComponents {
  /// Factory method for the creation of audio decoders. Will an audio decoder for the file at \c
  /// file_name. Will return \c nullptr if file opening failed.
  static WREDecodingComponents *get_audio_decoder(string file_name);

  virtual ~WREDecodingComponents();

  /// Decodes the next frame in the file into the internal \c frame.
  virtual int decode_next_frame();
  AVFormatContext *format_context;
};

/// Components needed in order to decode video from a given file using FFmpeg. This struct cleanly
/// wraps the frame decoding process, including handling framerate differences between the source
/// file and the framerate expected by the target video target.
struct WREVideoDecodingComponents : WREDecodingComponents {
  /// Factory method for the creation of audio decoders. Will an audio decoder for the file at \c
  /// file_name. Will return \c nullptr if file opening failed. \c expected_framerate will determine
  /// how many frames will be decoded per second of video, using a best-guess estimate that will
  /// drop or add frames, if the original video's framerate is different from \c expected_framerate.
  static WREVideoDecodingComponents *get_video_decoder(string file_name,
                                                       AVRational expected_framerate);

  ~WREVideoDecodingComponents();

  /// Decodes the next frame in the file into the internal \c frame.
  int decode_next_frame() override;

private:
  AVFrame *buffered_frame;
  long next_pts;
  long pts_increase_betweem_frames;
};
