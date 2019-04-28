#include "Encoder.hpp"

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/rational.h>
#include <libswscale/swscale.h>
#include "libavutil/imgutils.h"
}

#include "ConversionContext.hpp"
#include "FFmpegTranscoding.hpp"

static int prepare_video_encoder(TranscodingComponents *encoder, AVFormatContext *format_context,
                                 int width, int height, string codec_name,
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

  if (avcodec_parameters_from_context(encoder->stream->codecpar, encoder->context) < 0) {
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
  av_image_alloc(encoder->frame->data, encoder->frame->linesize, width, height, AV_PIX_FMT_YUV420P,
                 1);
  encoder->frame->pict_type = AV_PICTURE_TYPE_I;

  return 0;
}

static int prepare_audio_encoder(TranscodingComponents *encoder, AVFormatContext *format_context,
                                 TranscodingComponents *decoder) {
  encoder->stream = avformat_new_stream(format_context, NULL);
  encoder->codec = avcodec_find_encoder(decoder->codec->id);
  encoder->frame = decoder->frame;
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

  if (avcodec_parameters_to_context(encoder->context, encoder->stream->codecpar) < 0) {
    logging("could not copy encoder parameters to context");
    return -1;
  }

  if (avcodec_parameters_from_context(encoder->stream->codecpar, encoder->context) < 0) {
    logging("could not copy encoder parameters to output stream");
    return -1;
  }

  encoder->context->channel_layout = AV_CH_LAYOUT_STEREO;
  encoder->context->channels = av_get_channel_layout_nb_channels(encoder->context->channel_layout);

  if (avcodec_open2(encoder->context, encoder->codec, NULL) < 0) {
    logging("could not open the audio codec");
    return -1;
  }

  return 0;
}

Encoder::Encoder(string file_name, string video_codec_name, int video_width, int video_height,
                 double video_framerate, TranscodingComponents *audio_decoder) {
  video_encoder = new TranscodingComponents();
  audio_encoder = new TranscodingComponents();

  avformat_alloc_output_context2(&format_context, NULL, NULL, file_name.c_str());
  if (!format_context) {
    throw "could not allocate memory for output format";
  }

  if (prepare_video_encoder(video_encoder, format_context, video_width, video_height,
                            video_codec_name, av_d2q(video_framerate, 300)) != 0) {
    throw "error while preparing video encoder";
  }

  logging("audio encoder");
  if (prepare_audio_encoder(audio_encoder, format_context, audio_decoder)) {
    throw "error while preparing audio copy";
  }

  if (format_context->oformat->flags & AVFMT_GLOBALHEADER)
    format_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  if (!(format_context->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&format_context->pb, file_name.c_str(), AVIO_FLAG_WRITE) < 0) {
      throw "could not open the output file";
    }
  }
  if (avformat_write_header(format_context, NULL) < 0) {
    throw "an error occurred when opening output file";
  }
  video_conversion_context =
      ConversionContext::create_encoding_conversion_context(video_encoder->context);
}

static int encode_frame(TranscodingComponents *encoder, AVFormatContext *format_context,
                        AVRational source_time_base, AVFrame *frame) {
  AVCodecContext *codec_context = encoder->context;

  int ret;
  ret = avcodec_send_frame(codec_context, frame);

  while (ret >= 0) {
    ret = avcodec_receive_packet(codec_context, encoder->packet);

    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      break;
    } else if (ret < 0) {
      logging("ENCODING: Error while receiving a packet from the encoder: %s", av_err2str(ret));
      return -1;
    }

    encoder->packet->stream_index = encoder->stream->index;
    av_packet_rescale_ts(encoder->packet, source_time_base, encoder->stream->time_base);
    ret = av_interleaved_write_frame(format_context, encoder->packet);

    if (ret != 0) {
      logging("Error %d while receiving a packet from the decoder: %s", ret, av_err2str(ret));
    }
  }
  av_packet_unref(encoder->packet);
  return 0;
}

int Encoder::encode_video_frame(double source_time_base, long source_timestamp) {
  this->video_encoder->frame->pts = source_timestamp;
  this->video_conversion_context->convert_to_frame(this->video_encoder->frame);
  return encode_frame(video_encoder, format_context, av_d2q(source_time_base, 300),
                      video_encoder->frame);
}

int Encoder::encode_audio_frame(double source_time_base, long source_timestamp) {
  this->audio_encoder->frame->pts = source_timestamp;
  return encode_frame(audio_encoder, format_context, av_d2q(source_time_base, 300),
                      audio_encoder->frame);
}

int Encoder::finish_encoding() {
  encode_frame(video_encoder, format_context, video_encoder->stream->time_base, NULL);
  encode_frame(audio_encoder, format_context, audio_encoder->stream->time_base, NULL);
  av_write_trailer(format_context);
  return 0;
}

uint8_t *Encoder::get_rgb_buffer() {
  return this->video_conversion_context->get_rgb_buffer();
}

int Encoder::get_width() {
  return this->video_encoder->context->width;
}

int Encoder::get_height() {
  return this->video_encoder->context->height;
}

Encoder::~Encoder() {
  delete (video_encoder);
  delete (audio_encoder);
  delete (video_conversion_context);
  avformat_close_input(&format_context);
  avformat_free_context(format_context);
}
