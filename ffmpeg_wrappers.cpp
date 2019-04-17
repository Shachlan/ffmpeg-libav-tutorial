#include "ffmpeg_wrappers.hpp"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include "libavutil/imgutils.h"
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

void logging(const char *fmt, ...) {
  va_list args;
  fprintf(stderr, "LOG: ");
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
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

int prepare_audio_decoder(string file_name, DecodingComponents **decoder_ref) {
  DecodingComponents *decoder = new DecodingComponents();
  *decoder_ref = decoder;
  return prepare_decoding_components(decoder, file_name, AVMEDIA_TYPE_AUDIO);
}

int prepare_video_decoder(string file_name, AVRational expected_framerate,
                          VideoDecodingComponents **decoder_ref) {
  VideoDecodingComponents *decoder = new VideoDecodingComponents();
  if (prepare_decoding_components(decoder, file_name, AVMEDIA_TYPE_VIDEO) !=
      0) {
    return -1;
  }
  *decoder_ref = decoder;
  decoder->buffered_frame = av_frame_alloc();
  decoder->buffered_frame->pts = INT64_MIN;
  decoder->pts_increase_betweem_frames = (long)av_q2d(
      av_inv_q(av_mul_q(decoder->stream->time_base, expected_framerate)));
  decoder->next_pts = 0;
  decoder->file_name = file_name;
  return 0;
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

static int prepare_audio_copy(DecodingComponents *encoder,
                              DecodingComponents *decoder) {
  encoder->stream = avformat_new_stream(encoder->format_context, NULL);
  encoder->codec = avcodec_find_encoder(decoder->codec->id);
  encoder->frame = av_frame_alloc();
  encoder->packet = av_packet_alloc();
  avcodec_parameters_copy(encoder->stream->codecpar, decoder->stream->codecpar);
  if (!encoder->codec) {
    logging("could not find the proper codec");
    return -1;
  }

  encoder->context = avcodec_alloc_context3(encoder->codec);
  if (!encoder->context) {
    logging("could not allocated memory for codec context");
    return -1;
  }

  if (avcodec_parameters_to_context(encoder->context,
                                    encoder->stream->codecpar) < 0) {
    logging("could not copy encoder parameters to context");
    return -1;
  }

  if (avcodec_parameters_from_context(encoder->stream->codecpar,
                                      encoder->context) < 0) {
    logging("could not copy encoder parameters to output stream");
    return -1;
  }

  encoder->context->channel_layout = AV_CH_LAYOUT_STEREO;
  encoder->context->channels =
      av_get_channel_layout_nb_channels(encoder->context->channel_layout);

  if (avcodec_open2(encoder->context, encoder->codec, NULL) < 0) {
    logging("could not open the audio codec");
    return -1;
  }

  return 0;
}

int prepare_encoder(string file_name, AVRational expected_framerate,
                    DecodingComponents *decoder,
                    DecodingComponents **encoder_ref) {
  DecodingComponents *encoder = new DecodingComponents();
  encoder->file_name = file_name;
  avformat_alloc_output_context2(&encoder->format_context, NULL, NULL,
                                 file_name.c_str());
  if (!encoder->format_context) {
    logging("could not allocate memory for output format");
    return -1;
  }

  if (prepare_video_encoder(encoder, decoder, expected_framerate)) {
    logging("error while preparing video encoder");
    return -1;
  }

  // if (prepare_audio_copy(encoder, decoder))
  // {
  //     logging("error while preparing audio copy");
  //     return -1;
  // }

  if (encoder->format_context->oformat->flags & AVFMT_GLOBALHEADER)
    encoder->format_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  if (!(encoder->format_context->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&encoder->format_context->pb, encoder->file_name.c_str(),
                  AVIO_FLAG_WRITE) < 0) {
      logging("could not open the output file");
      return -1;
    }
  }
  if (avformat_write_header(encoder->format_context, NULL) < 0) {
    logging("an error occurred when opening output file");
    return -1;
  }

  *encoder_ref = encoder;
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

int get_next_video_frame(VideoDecodingComponents *decoder) {
  int result = 0;
  if (decoder->buffered_frame->pts >= decoder->next_pts) {
    // logging("%s passed frames: %lu:%lu", decoder->name, decoder->next_pts,
    // decoder->buffered_frame->pts);

    av_frame_copy(decoder->frame, decoder->buffered_frame);
    av_frame_copy_props(decoder->frame, decoder->buffered_frame);
    decoder->frame->pts = decoder->next_pts;
    decoder->next_pts += decoder->pts_increase_betweem_frames;

    // logging("got frame");
    return 0;
  }

  while (result >= 0) {
    av_packet_unref(decoder->packet);
    result = av_read_frame(decoder->format_context, decoder->packet);
    if (result == AVERROR_EOF) {
      break;
    }
    if (decoder->packet->stream_index != decoder->stream->index) {
      continue;
    }

    av_frame_unref(decoder->frame);
    result = decode_single_packet(decoder);
    if (result == AVERROR(EAGAIN)) {
      result = 0;
      continue;
    }

    av_frame_unref(decoder->buffered_frame);
    av_frame_copy(decoder->buffered_frame, decoder->frame);
    av_frame_copy_props(decoder->buffered_frame, decoder->frame);
    return get_next_video_frame(decoder);
  }
  return result;
}

int get_next_audio_frame(DecodingComponents *decoder) {
  int result = 0;
  while (result >= 0) {
    result = av_read_frame(decoder->format_context, decoder->packet);
    if (result == AVERROR_EOF) {
      break;
    }
    if (decoder->packet->stream_index != decoder->stream->index) {
      av_packet_unref(decoder->packet);
      continue;
    }

    av_frame_unref(decoder->frame);
    result = decode_single_packet(decoder);
    if (result == AVERROR(EAGAIN)) {
      result = 0;
      av_packet_unref(decoder->packet);
      continue;
    }
    break;
  }
  return result;
}

void free_context(DecodingComponents *context) {
  avcodec_close(context->context);
  avcodec_free_context(&context->context);
  avformat_close_input(&context->format_context);
  avformat_free_context(context->format_context);

  free(context);
}