// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "Transcoding/Encoder.hpp"

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include "libavutil/imgutils.h"
}

#include "Transcoding/AVRationalUtils.hpp"
#include "Transcoding/TranscodingComponents.hpp"
#include "Transcoding/TranscodingException.hpp"
#include "Transcoding/VideoFormatConverter.hpp"

using namespace WRETranscoding;

static int prepare_video_encoder(TranscodingComponents *encoder, AVFormatContext *format_context,
                                 int width, int height, std::string codec_name,
                                 AVRational expected_framerate) {
  encoder->stream = avformat_new_stream(format_context, NULL);
  encoder->codec = avcodec_find_encoder_by_name(codec_name.c_str());

  if (!encoder->codec) {
    log_error("could not find the proper codec");
    return -1;
  }

  encoder->context = create_codec_context(encoder->codec);
  if (!encoder->context) {
    log_error("could not allocated memory for codec context");
    return -1;
  }

  encoder->context->height = height;
  encoder->context->width = width;

  encoder->context->pix_fmt = encoder->codec->pix_fmts[0];

  if (avcodec_parameters_from_context(encoder->stream->codecpar, encoder->context.get()) < 0) {
    log_error("could not copy encoder parameters to output stream");
    return -1;
  }

  encoder->context->time_base = av_inv_q(expected_framerate);
  encoder->context->framerate = expected_framerate;
  encoder->stream->time_base = encoder->context->time_base;

  if (avcodec_open2(encoder->context.get(), encoder->codec, NULL) < 0) {
    log_error("could not open the codec");
    return -1;
  }

  encoder->frame = create_frame();
  encoder->packet = create_packet();
  encoder->frame->width = width;
  encoder->frame->height = height;
  encoder->frame->format = AV_PIX_FMT_YUV420P;
  av_image_alloc(encoder->frame->data, encoder->frame->linesize, width, height, AV_PIX_FMT_YUV420P,
                 1);
  encoder->frame->pict_type = AV_PICTURE_TYPE_I;

  return 0;
}

static int prepare_audio_encoder(TranscodingComponents *encoder, AVFormatContext *format_context,
                                 const TranscodingComponents *decoder) {
  encoder->stream = avformat_new_stream(format_context, NULL);
  encoder->codec = avcodec_find_encoder(decoder->codec->id);
  encoder->frame = decoder->frame;
  encoder->packet = create_packet();
  avcodec_parameters_copy(encoder->stream->codecpar, decoder->stream->codecpar);
  if (!encoder->codec) {
    log_error("could not find the proper codec");
    return -1;
  }

  encoder->context = create_codec_context(encoder->codec);
  if (!encoder->context) {
    log_error("could not allocated memory for codec context");
    return -1;
  }

  if (avcodec_parameters_to_context(encoder->context.get(), encoder->stream->codecpar) < 0) {
    log_error("could not copy encoder parameters to context");
    return -1;
  }

  if (avcodec_parameters_from_context(encoder->stream->codecpar, encoder->context.get()) < 0) {
    log_error("could not copy encoder parameters to output stream");
    return -1;
  }

  encoder->context->channel_layout = AV_CH_LAYOUT_STEREO;
  encoder->context->channels = av_get_channel_layout_nb_channels(encoder->context->channel_layout);

  if (avcodec_open2(encoder->context.get(), encoder->codec, NULL) < 0) {
    log_error("could not open the audio codec");
    return -1;
  }

  return 0;
}

Encoder::Encoder(std::string file_name, std::string video_codec_name, int video_width,
                 int video_height, double video_framerate,
                 const TranscodingComponents *audio_decoder) {
  log_info("Opening encoder for %s", file_name.c_str());
  video_encoder = std::make_unique<TranscodingComponents>();

  avformat_alloc_output_context2(&format_context, NULL, NULL, file_name.c_str());
  if (!format_context) {
    throw TranscodingException("could not allocate memory for output format");
  }

  if (prepare_video_encoder(video_encoder.get(), format_context, video_width, video_height,
                            video_codec_name, wre_double_to_rational(video_framerate)) != 0) {
    throw TranscodingException("error while preparing video encoder");
  }

  if (audio_decoder != nullptr) {
    audio_encoder = std::make_unique<TranscodingComponents>();
    if (prepare_audio_encoder(audio_encoder.get(), format_context, audio_decoder) != 0) {
      throw TranscodingException("error while preparing audio copy");
    }
  } else {
    audio_encoder = nullptr;
  }

  if (format_context->oformat->flags & AVFMT_GLOBALHEADER)
    format_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  if (!(format_context->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&format_context->pb, file_name.c_str(), AVIO_FLAG_WRITE) < 0) {
      throw TranscodingException("could not open the output file");
    }
  }
  if (avformat_write_header(format_context, NULL) < 0) {
    throw TranscodingException("an error occurred when opening output file");
  }
  video_conversion_context = std::make_unique<VideoFormatConverter>(
      video_encoder->context, VideoFormatConverter::EncodingConverionTag{});
}

static int encode_frame(TranscodingComponents *encoder, AVFormatContext *format_context,
                        AVRational source_time_base, AVFrame *frame) {
  encoder->latest_time_base = source_time_base;

  int ret;
  ret = avcodec_send_frame(encoder->context.get(), frame);

  while (ret >= 0) {
    ret = avcodec_receive_packet(encoder->context.get(), encoder->packet.get());

    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      break;
    } else if (ret < 0) {
      log_error("ENCODING: Error %d while receiving a packet from the encoder", ret);
      return -1;
    }

    encoder->packet->stream_index = encoder->stream->index;
    av_packet_rescale_ts(encoder->packet.get(), source_time_base, encoder->stream->time_base);
    ret = av_interleaved_write_frame(format_context, encoder->packet.get());

    if (ret != 0) {
      log_error("Error %d while writing a frame", ret);
    }
  }
  av_packet_unref(encoder->packet.get());
  return 0;
}

int Encoder::encode_video_frame(double source_time_base, long source_timestamp) noexcept {
  video_encoder->frame->pts = source_timestamp;
  video_conversion_context->convert_to_frame(video_encoder->frame);
  return encode_frame(video_encoder.get(), format_context, wre_double_to_rational(source_time_base),
                      video_encoder->frame.get());
}

int Encoder::encode_audio_frame(double source_time_base, long source_timestamp) noexcept {
  audio_encoder->frame->pts = source_timestamp;
  return encode_frame(audio_encoder.get(), format_context, wre_double_to_rational(source_time_base),
                      audio_encoder->frame.get());
}

int Encoder::finish_encoding() noexcept {
  log_info("finish encoding");
  encode_frame(video_encoder.get(), format_context, video_encoder->latest_time_base, NULL);
  if (audio_encoder != nullptr) {
    encode_frame(audio_encoder.get(), format_context, audio_encoder->latest_time_base, NULL);
  }
  av_write_trailer(format_context);
  return 0;
}

int Encoder::get_width() const noexcept {
  return video_encoder->context->width;
}

int Encoder::get_height() const noexcept {
  return video_encoder->context->height;
}

Encoder::~Encoder() {
  avformat_close_input(&format_context);
  avformat_free_context(format_context);
}
