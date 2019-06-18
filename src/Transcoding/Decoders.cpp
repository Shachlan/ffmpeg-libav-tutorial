// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "Transcoding/Decoders.hpp"

extern "C" {
#include <libavutil/mathematics.h>
}

#include "Transcoding/AVRationalUtils.hpp"
#include "Transcoding/TranscodingComponents.hpp"
#include "Transcoding/TranscodingException.hpp"
#include "Transcoding/VideoFormatConverter.hpp"

namespace WRETranscoding {

/// Components needed in order to decode media from a given file using FFmpeg. This struct cleanly
/// wraps the frame decoding process.
struct Decoder::Impl : TranscodingComponents {
  /// Factory method for the creation of audio decoders. Calling this will open an audio decoder
  /// for the file at \c file_name. Will return \c nullptr if file opening failed.
  ///
  /// @param file_name - the path to the file containing the audio stream.
  ///
  /// @param start_from - the point in time, in seconds, from which the audio stream should
  /// start. If \c start_from is larger than the duration of the stream, no frames will be
  /// decoded.
  ///
  /// @param duration - duration, in seconds, of the resulting decoded frames. If \c
  /// start_from + \c duration is larger than the duration of the stream, the decoder will
  /// decode until the end of the stream and stop, thus resulting in a shorter than requested
  /// duration.
  ///
  /// @param speed_ratio - the ratio between the speed of the original stream and of the
  /// decoded frames.
  Impl(string file_name, double start_from, double duration, double speed_ratio,
       AVMediaType media_type) {
    format_context = avformat_alloc_context();
    if (!format_context) {
      throw TranscodingException("could not allocate memory for Format Context");
    }

    int result = avformat_open_input(&format_context, file_name.c_str(), NULL, NULL);
    if (result != 0) {
      throw TranscodingException("could not open %s", file_name.c_str());
    }

    if (avformat_find_stream_info(format_context, NULL) < 0) {
      throw TranscodingException("could not get the stream info for %s", file_name.c_str());
    }

    int stream_index = av_find_best_stream(format_context, media_type, -1, -1, NULL, 0);
    if (stream_index < 0) {
      throw TranscodingException("could not get the stream index with error %d", stream_index);
    }

    stream = wrap_with_empty_deleter<AVStream>(format_context->streams[stream_index]);
    codec = wrap_with_empty_deleter<AVCodec>(avcodec_find_decoder(stream->codecpar->codec_id));
    context = create_codec_context(codec);
    if (avcodec_parameters_to_context(context.get(), stream->codecpar) < 0) {
      throw TranscodingException("failed to copy codec params to codec context");
    }

    if (avcodec_open2(context.get(), codec.get(), NULL) < 0) {
      throw TranscodingException("failed to open codec through avcodec_open2");
    }

    auto inverted_time_base = av_q2d(av_inv_q(stream->time_base));
    first_timestamp = 0;
    if (start_from > 0) {
      first_timestamp = start_from * inverted_time_base;
      log_info("seeking to: %ld", first_timestamp);
      int result = av_seek_frame(format_context, stream->index, first_timestamp,
                                 AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
      if (result != 0) {
        throw TranscodingException("Failed to seek in file %s to %ld", file_name.c_str(),
                                   first_timestamp);
      }
    }

    packet = create_packet();
    frame = create_frame();
    last_timestamp = (duration * inverted_time_base);
    adjusted_time_base = av_mul_q(stream->time_base, wre_double_to_rational(speed_ratio));
  }

  virtual ~Impl() {
    avformat_close_input(&format_context);
    avformat_free_context(format_context);
  }

  virtual bool should_log() {
    return false;
  }

  /// Decodes the next frame in the file into the internal \c frame.
  virtual int decode_next_frame() noexcept {
    int result = 0;
    while (result >= 0) {
      av_frame_unref(frame.get());
      result = avcodec_receive_frame(context.get(), frame.get());
      if (result == 0) {
        if (frame->pts > last_timestamp) {
          return AVERROR_EOF;
        }

        if (frame->pts < 0) {
          continue;
        }

        if (should_log()) {
          log_debug("decoding frame %lu", frame->pts);
        }
        return 0;
      }
      if (result != AVERROR(EAGAIN)) {
        // Only send more packets if the codec requires more packets.
        return result;
      }

      av_packet_unref(packet.get());
      auto packet_to_send = packet;
      result = av_read_frame(format_context, packet.get());
      packet->pts -= first_timestamp;
      av_packet_rescale_ts(packet.get(), stream->time_base, adjusted_time_base);
      if (result == AVERROR_EOF) {
        // Sending NULL to the codec context enters it into draining mode.
        packet_to_send = NULL;
      }
      if (packet_to_send != NULL && packet_to_send->stream_index != stream->index) {
        continue;
      }

      result = avcodec_send_packet(context.get(), packet.get());
    }

    return result;
  }

  /// Wrapper for the decoded file.
  AVFormatContext *format_context;

  /// Estimated timestamp of the first frame that will be decoded.
  long first_timestamp;

  /// Estimated timestamp of the last frame that will be decoded.
  long last_timestamp;

  /// Time base for the decoded stream after speed adjustments.
  AVRational adjusted_time_base;
};

/// Components needed in order to decode video from a given file using FFmpeg. This struct cleanly
/// wraps the frame decoding process, including handling framerate differences between the source
/// file and the framerate expected by the target video target.
struct VideoDecoderImplementation : Decoder::Impl {
  /// Factory method for the creation of video decoders. Calling this will open a video decoder
  /// for the file at \c file_name. Will return \c nullptr if file opening failed. \c
  /// expected_framerate will determine how many frames will be decoded per second of video,
  /// using a best-guess estimate that will drop or add frames, if the original video's
  /// framerate is different from \c expected_framerate.
  ///
  /// @param file_name - the path to the file containing the audio stream.
  ///
  /// @param expected_framerate - the framerate at which the frame should be decoded. If \c
  /// exptected_framerate will be higher than the framerate of the source stream, then some
  /// frames will be duplicted. If it is smaller, some frames will be dropped.
  ///
  /// @param start_from - the point in time, in seconds, from which the audio stream should
  /// start. If \c start_from is larger than the duration of the stream, no frames will be
  /// decoded.
  ///
  /// @param duration - duration, in seconds, of the resulting decoded frames. If \c
  /// start_from + \c duration is larger than the duration of the stream, the decoder will
  /// decode until the end of the stream and stop, thus resulting in a shorter than requested
  /// duration.
  ///
  /// @param speed_ratio - the ratio between the speed of the original stream and of the
  /// decoded frames.
  VideoDecoderImplementation(string file_name, AVRational expected_framerate, double start_from,
                             double duration, double speed_ratio)
      : Decoder::Impl(file_name, start_from, duration, speed_ratio, AVMEDIA_TYPE_VIDEO) {
    if (expected_framerate.num == 0) {
      expected_framerate = stream->avg_frame_rate;
    }
    if (expected_framerate.num == 0) {
      expected_framerate = stream->r_frame_rate;
    }

    latest_decoded_frame = create_frame();
    latest_decoded_frame->pts = INT64_MIN;
    pts_increase_between_frames =
        (long)av_q2d(av_inv_q(av_mul_q(stream->time_base, expected_framerate)));
    next_timestamp = 0;
  }

  bool should_log() override {
    return true;
  }

  /// Decodes the next frame in the file into the internal \c frame.
  int decode_next_frame() noexcept override {
    while (!latest_decoded_frame_fits_framerate()) {
      int result = Decoder::Impl::decode_next_frame();
      if (result == AVERROR_EOF && latest_decoded_frame_fits_framerate()) {
        break;
      }

      if (result != 0) {
        return result;
      }

      if (frame->pts >= next_timestamp) {
        av_frame_unref(latest_decoded_frame.get());
        copy_frame(latest_decoded_frame, frame);
      }
    }

    log_debug("sending frame %lu", frame->pts);
    copy_frame(frame, latest_decoded_frame);
    frame->pts = next_timestamp;
    next_timestamp += pts_increase_between_frames;
    return 0;
  }

private:
  void copy_frame(shared_ptr<AVFrame> dst, const shared_ptr<AVFrame> src) noexcept {
    av_frame_copy(dst.get(), src.get());
    av_frame_copy_props(dst.get(), src.get());
  }

  /// Returns \c true if \c latest_decoded_frame is within the time range of the next frame to be
  /// sent, as defined by \c expected_framerate. If \c false, another frame needs to be decoded.
  bool latest_decoded_frame_fits_framerate() const noexcept {
    return latest_decoded_frame->pts + pts_increase_between_frames > next_timestamp;
  }

  /// Latest read frame. This frame is saved in order to duplicate it, in case the expected frame
  /// rate requires it.
  shared_ptr<AVFrame> latest_decoded_frame;

  /// Timestamp of the next sent frame.
  long next_timestamp;

  /// Increase in pts of \c next_timestamp between sent frames.
  long pts_increase_between_frames;
};

#pragma region Decoder

long Decoder::get_current_timestamp() const noexcept {
  return decoder_implementation->frame->pts;
}

double Decoder::get_time_base() const noexcept {
  return av_q2d(decoder_implementation->stream->time_base);
}

double Decoder::get_duration() const noexcept {
  return av_q2d(av_mul_q(decoder_implementation->stream->time_base,
                         av_make_q(decoder_implementation->stream->duration, 1)));
}

Decoder::~Decoder() = default;

int Decoder::decode_next_frame() noexcept {
  return decoder_implementation->decode_next_frame();
}

#pragma endregion Decoder
#pragma region AudioDecoder

AudioDecoder::AudioDecoder(string file_name, double start_from, double duration) {
  log_info("Open audio decoder from: %s", file_name.c_str());
  decoder_implementation =
      std::make_unique<Decoder::Impl>(file_name, start_from, duration, 1, AVMEDIA_TYPE_AUDIO);
}

TranscodingComponents *AudioDecoder::get_transcoding_components() const noexcept {
  return decoder_implementation.get();
}

#pragma endregion AudioDecoder
#pragma region VideoDecoder

VideoDecoder::VideoDecoder(string file_name, double expected_framerate, double start_from,
                           double duration, double speed_ratio) {
  log_info("Open video decoder from: %s", file_name.c_str());
  decoder_implementation = std::make_unique<VideoDecoderImplementation>(
      file_name, wre_double_to_rational(expected_framerate), start_from, duration, speed_ratio);
  video_conversion_context = std::make_unique<VideoFormatConverter>(
      decoder_implementation->context, VideoFormatConverter::DecodingConverionTag{});
}

int VideoDecoder::get_width() const noexcept {
  return decoder_implementation->context->width;
}

int VideoDecoder::get_height() const noexcept {
  return decoder_implementation->context->height;
}

int VideoDecoder::decode_next_frame() noexcept {
  int result = Decoder::decode_next_frame();
  if (result == 0) {
    video_conversion_context->convert_from_frame(decoder_implementation->frame);
  }
  return result;
}

#pragma endregion VideoDecoder

}  // namespace WRETranscoding
