#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <libavutil/pixdesc.h>
#include "libavutil/imgutils.h"
#include <libswscale/swscale.h>
#include <string.h>
#include <inttypes.h>

#include "./preparation.h"
#include "./openGLShading.h"

static int encode_frame(TranscodeContext *decoder_context, TranscodeContext *encoder_context, AVFormatContext *format_context, AVCodecContext *codec_context, AVFrame *frame, int stream_index)
{
  AVPacket *output_packet = av_packet_alloc();
  if (!output_packet)
  {
    logging("could not allocate memory for output packet");
    return -1;
  }

  int ret;
  if (frame)
    logging("Send frame %3" PRId64 "", frame->pts);
  ret = avcodec_send_frame(codec_context, frame);

  while (ret >= 0)
  {
    ret = avcodec_receive_packet(codec_context, output_packet);

    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
      break;
    }
    else if (ret < 0)
    {
      logging("ENCODING: Error while receiving a packet from the encoder: %s", av_err2str(ret));
      return -1;
    }

    /* prepare packet for muxing */
    output_packet->stream_index = stream_index;

    av_packet_rescale_ts(output_packet,
                         decoder_context->stream[stream_index]->time_base,
                         encoder_context->stream[stream_index]->time_base);

    /* mux encoded frame */
    ret = av_interleaved_write_frame(format_context, output_packet);

    if (ret != 0)
    {
      logging("Error %d while receiving a packet from the decoder: %s", ret, av_err2str(ret));
    }

    logging("Write packet %d (size=%d)", output_packet->pts, output_packet->size);
  }
  av_packet_unref(output_packet);
  av_packet_free(&output_packet);
  return 0;
}

static int decode_packet(TranscodeContext *decoder_context, TranscodeContext *encoder_context,
                         AVPacket *packet, AVFrame *inputFrame, AVFrame *outputFrame, uint8_t **outputBuffer, int *lineSize,
                         struct SwsContext *yuv_to_rgb_ctx, struct SwsContext *rgb_to_yuv_ctx, int stream_index)
{
  AVCodecContext *codec_context = decoder_context->codec_context[stream_index];

  int response = avcodec_send_packet(codec_context, packet);

  if (response < 0)
  {
    logging("DECODER: Error while sending a packet to the decoder: %s", av_err2str(response));
    return response;
  }

  while (response >= 0)
  {
    response = avcodec_receive_frame(codec_context, inputFrame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
    {
      break;
    }
    else if (response < 0)
    {
      logging("DECODER: Error while receiving a frame from the decoder: %s", av_err2str(response));
      return response;
    }

    if (response >= 0)
    {
      if (codec_context->codec_type == AVMEDIA_TYPE_VIDEO)
      {
        logging(
            "\tEncoding VIDEO Frame %d (type=%c, size=%d bytes) pts %d key_frame %d [DTS %d]",
            codec_context->frame_number,
            av_get_picture_type_char(inputFrame->pict_type),
            inputFrame->pkt_size,
            inputFrame->pts,
            inputFrame->key_frame,
            inputFrame->coded_picture_number);

        sws_scale(yuv_to_rgb_ctx, (const uint8_t *const *)inputFrame->data, inputFrame->linesize, 0, inputFrame->height, outputBuffer, lineSize);
        //printf("invert frame\n");
        invertFrame(outputBuffer[0], inputFrame->width, inputFrame->height);
        //printf("rescale\n");
        sws_scale(rgb_to_yuv_ctx, (const uint8_t *const *)outputBuffer, lineSize, 0, inputFrame->height, outputFrame->data, outputFrame->linesize);
        //printf("encode frame\n");
        outputFrame->pts = inputFrame->pts;
        encode_frame(decoder_context, encoder_context, encoder_context->format_context, encoder_context->codec_context[stream_index], outputFrame, stream_index);
      }
      av_frame_unref(inputFrame);
    }
  }
  return 0;
}

int main(int argc, char *argv[])
{
  TranscodeContext *decoder_context = calloc(1, sizeof(TranscodeContext));
  decoder_context->file_name = argv[1];

  if (prepare_decoder(decoder_context))
  {
    logging("error while preparing input");
    return -1;
  }

  TranscodeContext *encoder_context = calloc(1, sizeof(TranscodeContext));
  encoder_context->file_name = argv[2];

  if (prepare_encoder(encoder_context, decoder_context))
  {
    logging("error while preparing output");
    return -1;
  }
  AVCodecContext *videoEncodingContext = encoder_context->codec_context[encoder_context->video_stream_index];
  int height = videoEncodingContext->height;
  int width = videoEncodingContext->width;
  setupOpenGL(width, height);
  struct SwsContext *yuv_to_rgb_ctx = sws_getContext(width, height, AV_PIX_FMT_YUV420P,
                                                     width, height, AV_PIX_FMT_RGB24,
                                                     SWS_BICUBIC, NULL, NULL, NULL);
  struct SwsContext *rgb_to_yuv_ctx = sws_getContext(width, height, AV_PIX_FMT_RGB24,
                                                     width, height, AV_PIX_FMT_YUV420P,
                                                     SWS_BICUBIC, NULL, NULL, NULL);

  //printf("initialize arrays\n");
  AVFrame *inputFrame = av_frame_alloc();
  AVFrame *outputFrame = av_frame_alloc();
  av_frame_copy_props(outputFrame, inputFrame);
  outputFrame->width = width;
  outputFrame->height = height;
  outputFrame->format = AV_PIX_FMT_YUV420P;
  av_image_alloc(outputFrame->data, outputFrame->linesize, width, height, AV_PIX_FMT_YUV420P, 1);
  outputFrame->pict_type = AV_PICTURE_TYPE_I;

  printf("pts: %d", outputFrame->pts);

  uint8_t *outputBuffer[1];
  outputBuffer[0] = calloc(3 * height * width, sizeof(uint8_t));
  int lineSize[] = {3 * width * sizeof(uint8_t), 0, 0, 0};

  if (!inputFrame)
  {
    logging("failed to allocated memory for AVFrame");
    return -1;
  }
  AVPacket *input_packet = av_packet_alloc();
  if (!input_packet)
  {
    logging("failed to allocated memory for AVPacket");
    return -1;
  }

  int response = 0;

  while (av_read_frame(decoder_context->format_context, input_packet) >= 0)
  {
    logging("AVPacket->pts %" PRId64, input_packet->pts);

    if (input_packet->stream_index == decoder_context->video_stream_index)
    {
      response = decode_packet(
          decoder_context,
          encoder_context,
          input_packet,
          inputFrame,
          outputFrame,
          outputBuffer,
          lineSize,
          yuv_to_rgb_ctx,
          rgb_to_yuv_ctx,
          input_packet->stream_index);

      if (response < 0)
        break;
      //if (--how_many_packets_to_process <= 0) break;
      av_packet_unref(input_packet);
    }
    else
    {
      // just copying audio stream
      av_packet_rescale_ts(input_packet,
                           decoder_context->stream[input_packet->stream_index]->time_base,
                           encoder_context->stream[input_packet->stream_index]->time_base);

      if (av_interleaved_write_frame(encoder_context->format_context, input_packet) < 0)
      {
        logging("error while copying audio stream");
        return -1;
      }
      logging("\tfinish copying packets without reencoding");
    }
  }
  // flush all frames
  encode_frame(decoder_context, encoder_context, encoder_context->format_context,
               encoder_context->codec_context[encoder_context->video_stream_index],
               NULL, encoder_context->video_stream_index);
  // should I do it for the audio stream too?

  av_write_trailer(encoder_context->format_context);

  logging("releasing all the resources");

  avformat_close_input(&decoder_context->format_context);
  avformat_free_context(decoder_context->format_context);
  av_packet_free(&input_packet);
  av_frame_free(&inputFrame);
  av_frame_free(&outputFrame);
  avcodec_free_context(&decoder_context->codec_context[0]);
  avcodec_free_context(&decoder_context->codec_context[1]);
  free(decoder_context);
  tearDownOpenGL();
  return 0;
}