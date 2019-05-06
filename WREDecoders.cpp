// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "WREDecoders.hpp"

extern "C" {
#include <libavutil/mathematics.h>
}

#include "WRETranscodingComponents.hpp"
#include "WREVideoFormatConverter.hpp"

/// Components needed in order to decode media from a given file using FFmpeg. This struct cleanly
/// wraps the frame decoding process.
struct WREDecodingComponents : WRETranscodingComponents {
  /// Factory method for the creation of audio decoders. Calling this will open an audio decoder for
  /// the file at \c file_name. Will return \c nullptr if file opening failed.
  static WREDecodingComponents *get_audio_decoder(string file_name, double start_from,
                                                  double duration, double speed_ratio);

  virtual ~WREDecodingComponents();

  /// Decodes the next frame in the file into the internal \c frame.
  virtual int decode_next_frame();
  AVFormatContext *format_context;
  long first_timestamp;
  long last_timestamp;
  AVRational adjusted_time_base;
};

/// Components needed in order to decode video from a given file using FFmpeg. This struct cleanly
/// wraps the frame decoding process, including handling framerate differences between the source
/// file and the framerate expected by the target video target.
struct WREVideoDecodingComponents : WREDecodingComponents {
  /// Factory method for the creation of video decoders. Calling this will open a video decoder for
  /// the file at \c file_name. Will return \c nullptr if file opening failed. \c expected_framerate
  /// will determine how many frames will be decoded per second of video, using a best-guess
  /// estimate that will drop or add frames, if the original video's framerate is different from \c
  /// expected_framerate.
  static WREVideoDecodingComponents *get_video_decoder(string file_name,
                                                       AVRational expected_framerate,
                                                       double start_from, double duration,
                                                       double speed_ratio);

  ~WREVideoDecodingComponents();

  /// Decodes the next frame in the file into the internal \c frame.
  int decode_next_frame() override;

private:
  AVFrame *buffered_frame;
  long next_pts;
  long pts_increase_betweem_frames;
};

WREDecodingComponents::~WREDecodingComponents() {
  avformat_close_input(&format_context);
  avformat_free_context(format_context);
}

WREVideoDecodingComponents::~WREVideoDecodingComponents() {
  av_frame_free(&buffered_frame);
}

int prepare_decoding_components(WREDecodingComponents *decoder, string file_name, AVMediaType media,
                                double start_from, double duration, double speed_ratio) {
  AVFormatContext *format_context = avformat_alloc_context();
  decoder->format_context = format_context;
  if (!format_context) {
    log_error("could not allocate memory for Format Context");
    return -1;
  }

  int result = avformat_open_input(&format_context, file_name.c_str(), NULL, NULL);
  if (result != 0) {
    log_error("could not open %s", file_name.c_str());
    return -1;
  }

  if (avformat_find_stream_info(format_context, NULL) < 0) {
    log_error("could not get the stream info");
    return -1;
  }

  int stream_index = av_find_best_stream(format_context, media, -1, -1, NULL, 0);
  if (stream_index < 0) {
    log_error("could not get the stream index");
    return -1;
  }
  AVStream *stream = format_context->streams[stream_index];

  decoder->stream = stream;
  decoder->codec = avcodec_find_decoder(stream->codecpar->codec_id);
  decoder->context = avcodec_alloc_context3(decoder->codec);
  if (avcodec_parameters_to_context(decoder->context, stream->codecpar) < 0) {
    log_error("failed to copy codec params to codec context");
    return -1;
  }

  if (avcodec_open2(decoder->context, decoder->codec, NULL) < 0) {
    log_error("failed to open codec through avcodec_open2");
    return -1;
  }

  auto inverted_time_base = av_q2d(av_inv_q(decoder->stream->time_base));
  decoder->first_timestamp = 0;
  if (start_from > 0) {
    decoder->first_timestamp = start_from * inverted_time_base;
    log_info("seeking to: %ld", decoder->first_timestamp);
    int result = av_seek_frame(decoder->format_context, decoder->stream->index,
                               decoder->first_timestamp, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
    if (result != 0) {
      return result;
    }
  }

  decoder->packet = av_packet_alloc();
  decoder->frame = av_frame_alloc();
  decoder->last_timestamp =
      decoder->first_timestamp + (duration * inverted_time_base * speed_ratio);
  decoder->adjusted_time_base = av_mul_q(decoder->stream->time_base, av_d2q(speed_ratio, 300));

  return 0;
}

WREDecodingComponents *WREDecodingComponents::get_audio_decoder(string file_name, double start_from,
                                                                double duration,
                                                                double speed_ratio) {
  WREDecodingComponents *decoder = new WREDecodingComponents();
  if (prepare_decoding_components(decoder, file_name, AVMEDIA_TYPE_AUDIO, start_from, duration,
                                  speed_ratio) != 0) {
    delete (decoder);
    return nullptr;
  }
  return decoder;
}

WREVideoDecodingComponents *WREVideoDecodingComponents::get_video_decoder(
    string file_name, AVRational expected_framerate, double start_from, double duration,
    double speed_ratio) {
  WREVideoDecodingComponents *decoder = new WREVideoDecodingComponents();
  if (prepare_decoding_components(decoder, file_name, AVMEDIA_TYPE_VIDEO, start_from, duration,
                                  speed_ratio) != 0) {
    delete (decoder);
    return nullptr;
  }
  if (expected_framerate.num == 0) {
    expected_framerate = decoder->stream->avg_frame_rate;
  }
  if (expected_framerate.num == 0) {
    expected_framerate = decoder->stream->r_frame_rate;
  }

  decoder->buffered_frame = av_frame_alloc();
  decoder->buffered_frame->pts = INT64_MIN;
  decoder->pts_increase_betweem_frames =
      (long)av_q2d(av_inv_q(av_mul_q(decoder->stream->time_base, expected_framerate)));
  decoder->next_pts = 0;
  return decoder;
}

void copy_frame(AVFrame *dst, const AVFrame *src) {
  av_frame_copy(dst, src);
  av_frame_copy_props(dst, src);
}

int WREVideoDecodingComponents::decode_next_frame() {
  if (this->buffered_frame->pts >= this->next_pts) {
    copy_frame(this->frame, this->buffered_frame);
    this->frame->pts = this->next_pts;
    this->next_pts += this->pts_increase_betweem_frames;
    return 0;
  }

  int result = WREDecodingComponents::decode_next_frame();
  if (result == 0) {
    av_frame_unref(this->buffered_frame);
    copy_frame(this->buffered_frame, this->frame);
    return this->decode_next_frame();
  } else if (result == AVERROR_EOF &&
             this->buffered_frame->pts + this->pts_increase_betweem_frames > this->next_pts) {
    copy_frame(this->frame, this->buffered_frame);
    this->next_pts += this->pts_increase_betweem_frames;
    return 0;
  }

  return result;
}

int WREDecodingComponents::decode_next_frame() {
  int result = 0;
  while (result >= 0) {
    av_frame_unref(this->frame);
    result = avcodec_receive_frame(this->context, this->frame);
    if (result == 0) {
      if (this->frame->pts > this->last_timestamp) {
        return AVERROR_EOF;
      }

      if (this->frame->pts < 0) {
        continue;
      }

      return 0;
    }
    if (result != AVERROR(EAGAIN)) {
      // Only send more packets if the codec requires more packets.
      return result;
    }

    av_packet_unref(this->packet);
    auto packet_to_send = this->packet;
    result = av_read_frame(this->format_context, this->packet);
    this->packet->pts -= this->first_timestamp;
    this->packet->dts -= this->first_timestamp;
    av_packet_rescale_ts(packet, this->stream->time_base, this->adjusted_time_base);
    if (result == AVERROR_EOF) {
      // Sending NULL to the codec context enters it into draining mode.
      packet_to_send = NULL;
    }
    if (packet_to_send != NULL && packet_to_send->stream_index != this->stream->index) {
      continue;
    }

    result = avcodec_send_packet(this->context, this->packet);
  }

  return result;
}

long WREDecoder::get_current_timestamp() {
  return internal_decoder->frame->pts;
}

double WREDecoder::get_time_base() {
  return av_q2d(internal_decoder->stream->time_base);
}

double WREDecoder::get_duration() {
  AVRational secondary_time_base = internal_decoder->adjusted_time_base;
  return av_q2d(av_mul_q(secondary_time_base, av_make_q(internal_decoder->stream->duration, 1)));
}

WREDecoder::~WREDecoder() {
  delete (internal_decoder);
}

WREAudioDecoder::WREAudioDecoder(string file_name, double start_from, double duration,
                                 double speed_ratio) {
  log_info("Open audio decoder from: %s", file_name.c_str());
  internal_decoder =
      WREDecodingComponents::get_audio_decoder(file_name, start_from, duration, speed_ratio);
}

WRETranscodingComponents *WREAudioDecoder::get_transcoding_components() {
  return internal_decoder;
}

int WREDecoder::decode_next_frame() {
  return internal_decoder->decode_next_frame();
}

WREVideoDecoder::WREVideoDecoder(string file_name, double expected_framerate, double start_from,
                                 double duration, double speed_ratio) {
  log_info("Open video decoder from: %s", file_name.c_str());
  internal_decoder = WREVideoDecodingComponents::get_video_decoder(
      file_name, av_d2q(expected_framerate, 300), start_from, duration, speed_ratio);
  video_conversion_context =
      WREVideoFormatConverter::create_decoding_conversion_context(internal_decoder->context);
}

WREVideoDecoder::~WREVideoDecoder() {
  delete (video_conversion_context);
}

int WREVideoDecoder::get_width() {
  return internal_decoder->context->width;
}

int WREVideoDecoder::get_height() {
  return internal_decoder->context->height;
}

uint8_t *WREVideoDecoder::get_rgb_buffer() {
  return this->video_conversion_context->get_rgb_buffer();
}

int WREVideoDecoder::decode_next_frame() {
  int result = internal_decoder->decode_next_frame();
  if (result == 0) {
    this->video_conversion_context->convert_from_frame(this->internal_decoder->frame);
  }
  return result;
}
