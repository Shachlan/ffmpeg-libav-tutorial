#include "ffmpeg_wrappers.hpp"

extern "C" {
#include "libavutil/imgutils.h"
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

TranscodingComponents::~TranscodingComponents() {
  av_frame_free(&frame);
  av_packet_free(&packet);
  avcodec_close(context);
  avcodec_free_context(&context);
}

DecodingComponents::~DecodingComponents() {
  avformat_close_input(&format_context);
  avformat_free_context(format_context);
}

VideoDecodingComponents::~VideoDecodingComponents() {
  av_frame_free(&buffered_frame);
}

int prepare_decoding_components(DecodingComponents *decoder, string file_name,
                                AVMediaType media) {
  decoder->file_name = file_name;
  AVFormatContext *format_context = avformat_alloc_context();
  decoder->format_context = format_context;
  if (!format_context) {
    logging("ERROR could not allocate memory for Format Context");
    return -1;
  }

  if (avformat_open_input(&format_context, decoder->file_name.c_str(), NULL,
                          NULL) != 0) {
    logging("ERROR could not open the file");
    return -1;
  }

  if (avformat_find_stream_info(format_context, NULL) < 0) {
    logging("ERROR could not get the stream info");
    return -1;
  }

  AVStream *stream = format_context->streams[av_find_best_stream(
      format_context, media, -1, -1, NULL, 0)];

  decoder->stream = stream;
  decoder->codec = avcodec_find_decoder(stream->codecpar->codec_id);
  decoder->context = avcodec_alloc_context3(decoder->codec);
  if (avcodec_parameters_to_context(decoder->context, stream->codecpar) < 0) {
    logging("failed to copy codec params to codec context");
    return -1;
  }

  if (avcodec_open2(decoder->context, decoder->codec, NULL) < 0) {
    logging("failed to open codec through avcodec_open2");
    return -1;
  }
  decoder->packet = av_packet_alloc();
  decoder->frame = av_frame_alloc();

  return 0;
}

DecodingComponents *DecodingComponents::get_audio_decoder(string file_name) {
  DecodingComponents *decoder = new DecodingComponents();
  if (prepare_decoding_components(decoder, file_name, AVMEDIA_TYPE_AUDIO) !=
      0) {
    delete (decoder);
    return nullptr;
  }
  return decoder;
}

VideoDecodingComponents *
VideoDecodingComponents::get_video_decoder(string file_name,
                                           AVRational expected_framerate) {
  VideoDecodingComponents *decoder = new VideoDecodingComponents();
  if (prepare_decoding_components(decoder, file_name, AVMEDIA_TYPE_VIDEO) !=
      0) {
    delete (decoder);
    return nullptr;
  }
  decoder->buffered_frame = av_frame_alloc();
  decoder->buffered_frame->pts = INT64_MIN;
  decoder->pts_increase_betweem_frames = (long)av_q2d(
      av_inv_q(av_mul_q(decoder->stream->time_base, expected_framerate)));
  decoder->next_pts = 0;
  decoder->file_name = file_name;
  return decoder;
}

static int prepare_video_encoder(DecodingComponents *encoder,
                                 DecodingComponents *decoder,
                                 AVRational expected_framerate) {
  encoder->stream = avformat_new_stream(encoder->format_context, NULL);
  encoder->codec = avcodec_find_encoder_by_name("libx264");

  if (!encoder->codec) {
    logging("could not find the proper codec");
    return -1;
  }

  encoder->context = avcodec_alloc_context3(encoder->codec);
  if (!encoder->context) {
    logging("could not allocated memory for codec context");
    return -1;
  }

  // how to free this?
  AVDictionary *encoder_options = NULL;

  AVCodecContext *encoder_codec_context = encoder->context;

  encoder_codec_context->height = decoder->context->height;
  encoder_codec_context->width = decoder->context->width;

  if (encoder->codec->pix_fmts)
    encoder->context->pix_fmt = encoder->codec->pix_fmts[0];
  else
    encoder->context->pix_fmt = decoder->context->pix_fmt;

  if (avcodec_parameters_from_context(encoder->stream->codecpar,
                                      encoder->context) < 0) {
    logging("could not copy encoder parameters to output stream");
    return -1;
  }

  encoder->context->time_base = decoder->stream->time_base;
  encoder->context->framerate = expected_framerate;
  encoder->stream->time_base = encoder->context->time_base;

  if (avcodec_open2(encoder->context, encoder->codec, &encoder_options) < 0) {
    logging("could not open the codec");
    return -1;
  }

  encoder->frame = av_frame_alloc();
  encoder->packet = av_packet_alloc();
  encoder->frame->width = decoder->context->width;
  encoder->frame->height = decoder->context->height;
  encoder->frame->format = AV_PIX_FMT_YUV420P;
  av_image_alloc(encoder->frame->data, encoder->frame->linesize,
                 decoder->context->width, decoder->context->height,
                 AV_PIX_FMT_YUV420P, 1);
  encoder->frame->pict_type = AV_PICTURE_TYPE_I;

  return 0;
}

static int decode_single_packet(DecodingComponents *decoder) {
  AVCodecContext *codec_context = decoder->context;
  int response = avcodec_send_packet(codec_context, decoder->packet);

  if (response < 0) {
    logging("DECODER: Error while sending a packet to the decoder: %s",
            av_err2str(response));
    return response;
  }

  return avcodec_receive_frame(codec_context, decoder->frame);
}

int VideoDecodingComponents::decode_next_video_frame() {
  int result = 0;
  if (this->buffered_frame->pts >= this->next_pts) {
    // logging("%s passed frames: %lu:%lu", decoder->name, decoder->next_pts,
    // decoder->buffered_frame->pts);

    av_frame_copy(this->frame, this->buffered_frame);
    av_frame_copy_props(this->frame, this->buffered_frame);
    this->frame->pts = this->next_pts;
    this->next_pts += this->pts_increase_betweem_frames;

    // logging("got frame");
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
    return this->decode_next_video_frame();
  }
  return result;
}

int DecodingComponents::decode_next_audio_frame() {
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