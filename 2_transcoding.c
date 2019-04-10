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
#include "./rationalExtensions.h"

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

static void invert_single_frame(AVFrame *inputFrame, AVFrame *outputFrame,
                                uint32_t textureID,
                                uint8_t **outputBuffer, int *lineSize,
                                struct SwsContext *input_conversion_context,
                                struct SwsContext *output_conversion_context)
{
  outputFrame->pts = inputFrame->pts;
  sws_scale(input_conversion_context, (const uint8_t *const *)inputFrame->data, inputFrame->linesize, 0, inputFrame->height, outputBuffer, lineSize);
  //printf("invert frame\n");
  loadTexture(textureID, inputFrame->width, inputFrame->height, outputBuffer[0]);
  invertFrame(textureID);
  getCurrentResults(inputFrame->width, inputFrame->height, outputBuffer[0]);
  //printf("rescale\n");
  sws_scale(output_conversion_context, (const uint8_t *const *)outputBuffer, lineSize, 0, inputFrame->height, outputFrame->data, outputFrame->linesize);
}

static void blend_frames(AVFrame *inputFrame, AVFrame *secondary_input_frame,
                         AVFrame *outputFrame, uint32_t texture1ID, uint32_t texture2ID,
                         uint8_t **outputBuffer, int *lineSize, uint8_t **secondary_buffer, int *secondary_linesize,
                         struct SwsContext *input_conversion_context, struct SwsContext *output_conversion_context, struct SwsContext *secondary_conversion_context)
{
  outputFrame->pts = inputFrame->pts;
  sws_scale(input_conversion_context, (const uint8_t *const *)inputFrame->data, inputFrame->linesize, 0, inputFrame->height, outputBuffer, lineSize);
  sws_scale(secondary_conversion_context, (const uint8_t *const *)secondary_input_frame->data, secondary_input_frame->linesize, 0, secondary_input_frame->height, secondary_buffer, secondary_linesize);
  //printf("invert frame\n");
  loadTexture(texture1ID, inputFrame->width, inputFrame->height, outputBuffer[0]);
  loadTexture(texture2ID, secondary_input_frame->width, secondary_input_frame->height, secondary_buffer[0]);
  blendFrames(texture1ID, texture2ID);
  getCurrentResults(inputFrame->width, inputFrame->height, outputBuffer[0]);
  //printf("rescale\n");
  sws_scale(output_conversion_context, (const uint8_t *const *)outputBuffer, lineSize, 0, inputFrame->height, outputFrame->data, outputFrame->linesize);
}

static int decode_single_packet(TranscodeContext *decoder_context, AVPacket *packet, AVFrame *inputFrame, int stream_index)
{
  AVCodecContext *codec_context = decoder_context->codec_context[stream_index];
  int response = avcodec_send_packet(codec_context, packet);

  if (response < 0)
  {
    logging("DECODER: Error while sending a packet to the decoder: %s", av_err2str(response));
    return response;
  }

  return avcodec_receive_frame(codec_context, inputFrame);
}

struct SwsContext *conversion_context_from_codec_to_rgb(AVCodecContext *context)
{
  int height = context->height;
  int width = context->width;
  return sws_getContext(width, height, context->pix_fmt,
                        width, height, AV_PIX_FMT_RGB24,
                        SWS_BICUBIC, NULL, NULL, NULL);
}

struct SwsContext *conversion_context_from_rgb_to_codec(AVCodecContext *context)
{
  int height = context->height;
  int width = context->width;
  return sws_getContext(width, height, AV_PIX_FMT_RGB24,
                        width, height, context->pix_fmt,
                        SWS_BICUBIC, NULL, NULL, NULL);
}

int *linesize_for_size(int width)
{
  int *linesize = calloc(4, sizeof(int));
  linesize[0] = 3 * width * sizeof(uint8_t);
  return linesize;
}

int *linesize_for_codec(AVCodecContext *context)
{
  return linesize_for_size(context->width);
}

uint8_t **rgb_buffer_for_size(int width, int height)
{
  uint8_t **buffer = calloc(1, sizeof(uint8_t *));
  buffer[0] = calloc(3 * height * width, sizeof(uint8_t));
  return buffer;
}

uint8_t **rgb_buffer_for_codec(AVCodecContext *context)
{
  return rgb_buffer_for_size(context->width, context->height);
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

  TranscodeContext *secondary_decoder = calloc(1, sizeof(TranscodeContext));
  secondary_decoder->file_name = argv[2];

  if (prepare_decoder(secondary_decoder))
  {
    logging("error while preparing secondary input");
    return -1;
  }

  TranscodeContext *encoder_context = calloc(1, sizeof(TranscodeContext));
  encoder_context->file_name = argv[3];

  if (prepare_encoder(encoder_context, decoder_context))
  {
    logging("error while preparing output");
    return -1;
  }

  float blendRatio = strtof(argv[4], NULL);
  printf("blend ratio: %f\n", blendRatio);
  int wait = atoi(argv[5]);

  AVCodecContext *videoEncodingContext = encoder_context->codec_context[encoder_context->video_stream_index];
  AVCodecContext *secondary_video_context = secondary_decoder->codec_context[secondary_decoder->video_stream_index];
  int height = videoEncodingContext->height;
  int width = videoEncodingContext->width;
  setupOpenGL(width, height, blendRatio, NULL);
  uint32_t tex1 = createTexture();
  uint32_t tex2 = createTexture();
  struct SwsContext *input_conversion_context = conversion_context_from_codec_to_rgb(videoEncodingContext);
  struct SwsContext *secondary_input_conversion_context = conversion_context_from_codec_to_rgb(secondary_video_context);
  struct SwsContext *output_conversion_context = conversion_context_from_rgb_to_codec(videoEncodingContext);
  AVStream *secondary_video_stream = secondary_decoder->stream[secondary_decoder->video_stream_index];

  //printf("initialize arrays\n");
  AVFrame *inputFrame = av_frame_alloc();
  AVFrame *secondary_input_frame = av_frame_alloc();
  AVFrame *outputFrame = av_frame_alloc();
  av_frame_copy_props(outputFrame, inputFrame);
  outputFrame->width = width;
  outputFrame->height = height;
  outputFrame->format = AV_PIX_FMT_YUV420P;
  av_image_alloc(outputFrame->data, outputFrame->linesize, width, height, AV_PIX_FMT_YUV420P, 1);
  outputFrame->pict_type = AV_PICTURE_TYPE_I;

  printf("allocating arrays\n");
  uint8_t **outputBuffer = rgb_buffer_for_codec(videoEncodingContext);
  uint8_t **secondary_buffer = rgb_buffer_for_codec(secondary_video_context);
  int *lineSize = linesize_for_codec(videoEncodingContext);
  int *secondary_lineSize = linesize_for_codec(secondary_video_context);
  printf("finished allocating arrays\n");

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

  AVRational time_base = decoder_context->stream[decoder_context->video_stream_index]->time_base;
  AVRational secondary_time_base = secondary_video_stream->time_base;
  double duration_in_seconds = av_q2d(multiply_by_int(secondary_time_base, secondary_video_stream->duration));
  double maxTime = duration_in_seconds + wait;
  printf("duration: %lf, max time: %lf, wait: %d\n", duration_in_seconds, maxTime, wait);
  AVRational last_secondary_pts = av_make_q(INT16_MIN, 1);
  AVRational next_secondary_pts = av_make_q(0, 1);

  while (av_read_frame(decoder_context->format_context, input_packet) >= 0)
  {
    logging("AVPacket->pts %" PRId64, input_packet->pts);

    if (input_packet->stream_index == decoder_context->audio_stream_index)
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
      av_packet_unref(input_packet);
      continue;
    }

    int response = decode_single_packet(
        decoder_context, input_packet,
        inputFrame, input_packet->stream_index);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
    {
      continue;
    }
    else if (response < 0)
    {
      logging("DECODER: Error while receiving a frame from the decoder: %s", av_err2str(response));
      return response;
    }
    AVRational current_pos = multiply_by_int(time_base, input_packet->pts);
    av_packet_unref(input_packet);
    double point_in_time = av_q2d(current_pos);

    if (point_in_time < wait || point_in_time > maxTime)
    {
      invert_single_frame(inputFrame, outputFrame, tex1, outputBuffer, lineSize, input_conversion_context, output_conversion_context);
      encode_frame(decoder_context, encoder_context, encoder_context->format_context, encoder_context->codec_context[input_packet->stream_index], outputFrame, input_packet->stream_index);
      av_frame_unref(inputFrame);
      continue;
    }

    AVRational adjustedPos = subtract_int(current_pos, wait);
    if (av_nearer_q(adjustedPos, last_secondary_pts, next_secondary_pts) == -1)
    {
      while (av_read_frame(secondary_decoder->format_context, input_packet) >= 0)
      {
        if (input_packet->stream_index != secondary_video_stream->index)
        {
          av_packet_unref(input_packet);
          continue;
        }
        int response = decode_single_packet(
            secondary_decoder, input_packet,
            secondary_input_frame, input_packet->stream_index);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
          continue;
        }
        if (response < 0)
        {
          logging("DECODER: Error while receiving a frame from the decoder: %s", av_err2str(response));
          return response;
        }

        last_secondary_pts = multiply_by_int(secondary_time_base, input_packet->pts);
        next_secondary_pts = multiply_by_int(secondary_time_base, input_packet->pts + input_packet->duration);
        av_packet_unref(input_packet);
        break;
      }
    }

    blend_frames(inputFrame, secondary_input_frame, outputFrame, tex1, tex2,
                 outputBuffer, lineSize, secondary_buffer, secondary_lineSize,
                 input_conversion_context, output_conversion_context, secondary_input_conversion_context);
    encode_frame(decoder_context, encoder_context, encoder_context->format_context, encoder_context->codec_context[input_packet->stream_index], outputFrame, input_packet->stream_index);
    av_frame_unref(inputFrame);
    av_frame_unref(secondary_input_frame);
    //if (--how_many_packets_to_process <= 0) break;
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
  avformat_close_input(&secondary_decoder->format_context);
  avformat_free_context(secondary_decoder->format_context);
  av_packet_free(&input_packet);
  av_frame_free(&inputFrame);
  av_frame_free(&outputFrame);
  avcodec_free_context(&decoder_context->codec_context[0]);
  avcodec_free_context(&decoder_context->codec_context[1]);
  free(decoder_context);
  tearDownOpenGL();
  return 0;
}