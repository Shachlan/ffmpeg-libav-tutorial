#include "FFmpegTranscoding.hpp"

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include "libavutil/imgutils.h"
}

WRETranscodingComponents::~WRETranscodingComponents() {
  av_frame_free(&frame);
  av_packet_free(&packet);
  avcodec_close(context);
  avcodec_free_context(&context);
}

WREDecodingComponents::~WREDecodingComponents() {
  avformat_close_input(&format_context);
  avformat_free_context(format_context);
}

WREVideoDecodingComponents::~WREVideoDecodingComponents() {
  av_frame_free(&buffered_frame);
}

int prepare_decoding_components(WREDecodingComponents *decoder, string file_name,
                                AVMediaType media) {
  AVFormatContext *format_context = avformat_alloc_context();
  decoder->format_context = format_context;
  if (!format_context) {
    log_error("ERROR could not allocate memory for Format Context");
    return -1;
  }

  if (avformat_open_input(&format_context, file_name.c_str(), NULL, NULL) != 0) {
    log_error("ERROR could not open %s", file_name.c_str());
    return -1;
  }

  if (avformat_find_stream_info(format_context, NULL) < 0) {
    log_error("ERROR could not get the stream info");
    return -1;
  }

  AVStream *stream =
      format_context->streams[av_find_best_stream(format_context, media, -1, -1, NULL, 0)];

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
  decoder->packet = av_packet_alloc();
  decoder->frame = av_frame_alloc();

  return 0;
}

WREDecodingComponents *WREDecodingComponents::get_audio_decoder(string file_name) {
  WREDecodingComponents *decoder = new WREDecodingComponents();
  if (prepare_decoding_components(decoder, file_name, AVMEDIA_TYPE_AUDIO) != 0) {
    delete (decoder);
    return nullptr;
  }
  return decoder;
}

WREVideoDecodingComponents *WREVideoDecodingComponents::get_video_decoder(
    string file_name, AVRational expected_framerate) {
  WREVideoDecodingComponents *decoder = new WREVideoDecodingComponents();
  if (prepare_decoding_components(decoder, file_name, AVMEDIA_TYPE_VIDEO) != 0) {
    delete (decoder);
    return nullptr;
  }
  decoder->buffered_frame = av_frame_alloc();
  decoder->buffered_frame->pts = INT64_MIN;
  decoder->pts_increase_betweem_frames =
      (long)av_q2d(av_inv_q(av_mul_q(decoder->stream->time_base, expected_framerate)));
  decoder->next_pts = 0;
  return decoder;
}

static int decode_single_packet(WREDecodingComponents *decoder) {
  AVCodecContext *codec_context = decoder->context;
  int response = avcodec_send_packet(codec_context, decoder->packet);

  if (response < 0) {
    log_error("DECODER: Error while sending a packet to the decoder: %s", av_err2str(response));
    return response;
  }

  return avcodec_receive_frame(codec_context, decoder->frame);
}

int WREVideoDecodingComponents::decode_next_frame() {
  int result = 0;
  if (this->buffered_frame->pts >= this->next_pts) {
    av_frame_copy(this->frame, this->buffered_frame);
    av_frame_copy_props(this->frame, this->buffered_frame);
    this->frame->pts = this->next_pts;
    this->next_pts += this->pts_increase_betweem_frames;
    return 0;
  }

  while (result >= 0) {
    av_packet_unref(this->packet);
    result = av_read_frame(this->format_context, this->packet);
    if (result == AVERROR_EOF) {
      break;
    }
    if (this->packet->stream_index != this->stream->index) {
      continue;
    }

    av_frame_unref(this->frame);
    result = decode_single_packet(this);
    if (result == AVERROR(EAGAIN)) {
      result = 0;
      continue;
    }

    av_frame_unref(this->buffered_frame);
    av_frame_copy(this->buffered_frame, this->frame);
    av_frame_copy_props(this->buffered_frame, this->frame);
    return this->decode_next_frame();
  }
  return result;
}

int WREDecodingComponents::decode_next_frame() {
  int result = 0;
  while (result >= 0) {
    result = av_read_frame(this->format_context, this->packet);
    if (result == AVERROR_EOF) {
      break;
    }
    if (this->packet->stream_index != this->stream->index) {
      av_packet_unref(this->packet);
      continue;
    }

    av_frame_unref(this->frame);
    result = decode_single_packet(this);
    if (result == AVERROR(EAGAIN)) {
      result = 0;
      av_packet_unref(this->packet);
      continue;
    }
    break;
  }
  return result;
}
