#include "Encoder.hpp"

#include "ffmpeg_wrappers.hpp"

extern "C" {
#include "libavutil/imgutils.h"
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/rational.h>
#include <libswscale/swscale.h>
}

static int prepare_video_encoder(TranscodingComponents *encoder,
                                 AVFormatContext *format_context, int width,
                                 int height, string codec_name,
                                 AVRational expected_framerate) {
  encoder->stream = avformat_new_stream(format_context, NULL);
  encoder->codec = avcodec_find_encoder_by_name(codec_name.c_str());

  if (!encoder->codec) {
    logging("could not find the proper codec");
    return -1;
  }

  encoder->context = avcodec_alloc_context3(encoder->codec);
  if (!encoder->context) {
    logging("could not allocated memory for codec context");
    return -1;
  }

  AVCodecContext *encoder_codec_context = encoder->context;

  encoder_codec_context->height = height;
  encoder_codec_context->width = width;

  encoder->context->pix_fmt = encoder->codec->pix_fmts[0];

  if (avcodec_parameters_from_context(encoder->stream->codecpar,
                                      encoder->context) < 0) {
    logging("could not copy encoder parameters to output stream");
    return -1;
  }

  encoder->context->time_base = av_inv_q(expected_framerate);
  encoder->context->framerate = expected_framerate;
  encoder->stream->time_base = encoder->context->time_base;

  if (avcodec_open2(encoder->context, encoder->codec, NULL) < 0) {
    logging("could not open the codec");
    return -1;
  }

  encoder->frame = av_frame_alloc();
  encoder->packet = av_packet_alloc();
  encoder->frame->width = width;
  encoder->frame->height = height;
  encoder->frame->format = AV_PIX_FMT_YUV420P;
  av_image_alloc(encoder->frame->data, encoder->frame->linesize, width, height,
                 AV_PIX_FMT_YUV420P, 1);
  encoder->frame->pict_type = AV_PICTURE_TYPE_I;

  return 0;
}

static int prepare_audio_encoder(TranscodingComponents *encoder,
                                 AVFormatContext *format_context,
                                 string codec_name, int sample_rate,
                                 long bit_rate) {
  encoder->stream = avformat_new_stream(format_context, NULL);
  encoder->frame = av_frame_alloc();
  encoder->packet = av_packet_alloc();
  encoder->codec = avcodec_find_encoder_by_name(codec_name.c_str());
  if (!encoder->codec) {
    logging("could not find the proper codec");
    return -1;
  }

  encoder->context = avcodec_alloc_context3(encoder->codec);
  if (!encoder->context) {
    logging("could not allocated memory for codec context");
    return -1;
  }

  encoder->stream->time_base = av_make_q(1, sample_rate);
  encoder->context->channel_layout = AV_CH_LAYOUT_STEREO;
  encoder->context->channels =
      av_get_channel_layout_nb_channels(encoder->context->channel_layout);
  encoder->context->sample_rate = sample_rate;
  encoder->context->sample_fmt = encoder->codec->sample_fmts[0];
  encoder->context->bit_rate = bit_rate;

  if (avcodec_parameters_from_context(encoder->stream->codecpar,
                                      encoder->context) < 0) {
    logging("could not copy encoder parameters to output stream");
    return -1;
  }

  if (avcodec_open2(encoder->context, encoder->codec, NULL) < 0) {
    logging("could not open the audio codec");
    return -1;
  }

  return 0;
}

Encoder::Encoder(string file_name, string video_codec_name, int video_width,
                 int video_height, double video_framerate,
                 string audio_codec_name, int audio_sample_rate,
                 long audio_bit_rate) {
  video_encoder = new TranscodingComponents();
  audio_encoder = new TranscodingComponents();

  avformat_alloc_output_context2(&format_context, NULL, NULL,
                                 file_name.c_str());
  if (!format_context) {
    throw "could not allocate memory for output format";
  }

  if (prepare_video_encoder(video_encoder, format_context, video_width,
                            video_height, video_codec_name,
                            av_d2q(video_framerate, 300)) != 0) {
    throw "error while preparing video encoder";
  }

  logging("audio encoder");
  if (prepare_audio_encoder(audio_encoder, format_context, audio_codec_name,
                            audio_sample_rate, audio_bit_rate)) {
    throw "error while preparing audio copy";
  }

  if (format_context->oformat->flags & AVFMT_GLOBALHEADER)
    format_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  if (!(format_context->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&format_context->pb, file_name.c_str(), AVIO_FLAG_WRITE) <
        0) {
      throw "could not open the output file";
    }
  }
  if (avformat_write_header(format_context, NULL) < 0) {
    throw "an error occurred when opening output file";
  }
}

static int encode_frame(TranscodingComponents *encoder,
                        AVFormatContext *format_context,
                        AVRational source_time_base, AVFrame *frame) {
  AVCodecContext *codec_context = encoder->context;

  int ret;
  ret = avcodec_send_frame(codec_context, frame);

  while (ret >= 0) {
    ret = avcodec_receive_packet(codec_context, encoder->packet);

    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      break;
    } else if (ret < 0) {
      logging("ENCODING: Error while receiving a packet from the encoder: %s",
              av_err2str(ret));
      return -1;
    }

    /* prepare packet for muxing */
    encoder->packet->stream_index = encoder->stream->index;

    av_packet_rescale_ts(encoder->packet, source_time_base,
                         encoder->stream->time_base);

    /* mux encoded frame */
    ret = av_interleaved_write_frame(format_context, encoder->packet);

    if (ret != 0) {
      logging("Error %d while receiving a packet from the decoder: %s", ret,
              av_err2str(ret));
    }

    // logging("Write packet %d (size=%d)", output_packet->pts,
    // output_packet->size);
  }
  av_packet_unref(encoder->packet);
  return 0;
}

int Encoder::encode_video_frame(double source_time_base) {
  return encode_frame(video_encoder, format_context,
                      av_d2q(source_time_base, 300), video_encoder->frame);
}

int Encoder::encode_audio_frame(double source_time_base) {
  return encode_frame(audio_encoder, format_context,
                      av_d2q(source_time_base, 300), audio_encoder->frame);
}

int Encoder::finish_encoding() {
  encode_frame(video_encoder, format_context, video_encoder->stream->time_base,
               NULL);
  encode_frame(audio_encoder, format_context, audio_encoder->stream->time_base,
               NULL);
  av_write_trailer(format_context);
}
