#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
extern "C" {
#include <libavutil/pixdesc.h>
#include "libavutil/imgutils.h"
#include <libswscale/swscale.h>
#include "./rationalExtensions.h"
#include "./video_conversion_utilities.h"
}
#include <string.h>
#include <inttypes.h>
#include "./openGLShading.h"

#define FRONTEND 0;

static void invert_single_frame(AVFrame *inputFrame, AVFrame *outputFrame,
                                uint32_t textureID,
                                ConversionContext *decoding_conversion,
                                ConversionContext *encoding_conversion)
{
  outputFrame->pts = inputFrame->pts;
  convert_from_frame(inputFrame, decoding_conversion);
  //printf("invert frame\n");
  loadTexture(textureID, inputFrame->width, inputFrame->height, decoding_conversion->rgb_buffer[0]);
  invertFrame(textureID);
  getCurrentResults(outputFrame->width, outputFrame->height, encoding_conversion->rgb_buffer[0]);
  convert_to_frame(outputFrame, encoding_conversion);
}

static void blend_frames(AVFrame *inputFrame, AVFrame *secondary_input_frame, AVFrame *outputFrame,
                         uint32_t texture1ID, uint32_t texture2ID,
                         ConversionContext *decoding_conversion,
                         ConversionContext *secondary_decoding_conversion,
                         ConversionContext *encoding_conversion)
{
  outputFrame->pts = inputFrame->pts;
  convert_from_frame(inputFrame, decoding_conversion);
  convert_from_frame(secondary_input_frame, secondary_decoding_conversion);
  //printf("invert frame\n");
  loadTexture(texture1ID, inputFrame->width, inputFrame->height, decoding_conversion->rgb_buffer[0]);
  loadTexture(texture2ID, secondary_input_frame->width, secondary_input_frame->height, secondary_decoding_conversion->rgb_buffer[0]);
  blendFrames(texture1ID, texture2ID);
  getCurrentResults(outputFrame->width, outputFrame->height, encoding_conversion->rgb_buffer[0]);
  //printf("rescale\n");
  convert_to_frame(outputFrame, encoding_conversion);
}

int main(int argc, char *argv[])
{
  TranscodeContext *decoder;
  TranscodeContext *secondary_decoder;
  TranscodeContext *encoder;
  AVRational expected_framerate = av_make_q(30, 1);

  if (prepare_decoder(argv[1], expected_framerate, &decoder))
  {
    logging("error while preparing input");
    return -1;
  }

  if (prepare_decoder(argv[2], expected_framerate, &secondary_decoder))
  {
    logging("error while preparing secondary input");
    return -1;
  }

  if (prepare_encoder(argv[3], expected_framerate, decoder, &encoder))
  {
    logging("error while preparing output");
    return -1;
  }
  decoder->video->name = "main";
  secondary_decoder->video->name = "secondary";

  float blendRatio = strtof(argv[4], NULL);
  printf("blend ratio: %f\n", blendRatio);
  int wait = atoi(argv[5]);

  AVCodecContext *video_encoding_context = encoder->video->context;
  AVCodecContext *secondary_video_context = secondary_decoder->video->context;
  int height = video_encoding_context->height;
  int width = video_encoding_context->width;
  setupOpenGL(width, height, blendRatio, NULL);
  uint32_t tex1 = createTexture();
  uint32_t tex2 = createTexture();
  ConversionContext *input_conversion_context = create_decoding_conversion_context(decoder->video->context);
  ConversionContext *secondary_input_conversion_context = create_decoding_conversion_context(secondary_video_context);
  ConversionContext *encoding_conversion = create_encoding_conversion_context(video_encoding_context);

  AVStream *secondary_video_stream = secondary_decoder->video->stream;

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

  AVRational time_base = decoder->video->stream->time_base;
  AVRational secondary_time_base = secondary_video_stream->time_base;
  double duration_in_seconds = av_q2d(multiply_by_int(secondary_time_base, secondary_video_stream->duration));
  double maxTime = duration_in_seconds + wait;
  printf("duration: %lf, max time: %lf, wait: %d\n", duration_in_seconds, maxTime, wait);
  int response;

  long counted_frames = 0;
  AVMediaType media_type;
  while (get_next_frame(decoder, input_packet, inputFrame, &media_type) >= 0)
  {
    bool video_frame = media_type == AVMEDIA_TYPE_VIDEO;
    FrameCodingComponents *components = video_frame ? decoder->video : decoder->audio;
    if (!video_frame)
    {
      //logging("audio frame %lu", input_packet->pts);
      av_packet_unref(input_packet);
      response = encode_frame(components, encoder->audio, inputFrame, input_packet, encoder->format_context);
      if (response < 0)
      {
        logging("DECODER: Error while receiving an audio frame from the decoder: %s", av_err2str(response));
        return response;
      }
      av_frame_unref(inputFrame);
      continue;
    }
    counted_frames++;
    //logging("video frame %lu", input_packet->pts);
    AVRational current_pos = multiply_by_int(time_base, inputFrame->pts);
    av_packet_unref(input_packet);
    double point_in_time = av_q2d(current_pos);
    //logging("point in time %f", point_in_time);
    if (point_in_time < wait || point_in_time > maxTime)
    {
      invert_single_frame(inputFrame, outputFrame, tex1,
                          input_conversion_context, encoding_conversion);
      encode_frame(components, encoder->video, outputFrame, input_packet, encoder->format_context);
      //av_frame_unref(inputFrame);
      continue;
    }

    response = get_next_video_frame(secondary_decoder, input_packet, secondary_input_frame);
    if (response < 0 && response != AVERROR_EOF)
    {
      logging("DECODER: Error while receiving a frame from the secondary decoder: %d %s", response, av_err2str(response));
      return response;
    }
    else if (response != AVERROR_EOF)
    {
      invert_single_frame(inputFrame, outputFrame, tex1,
                          input_conversion_context, encoding_conversion);
      encode_frame(components, encoder->video, outputFrame, input_packet, encoder->format_context);
      continue;
    }

    av_packet_unref(input_packet);

    blend_frames(inputFrame, secondary_input_frame, outputFrame, tex1, tex2,
                 input_conversion_context, secondary_input_conversion_context, encoding_conversion);
    encode_frame(components, encoder->video, outputFrame, input_packet, encoder->format_context);
    //if (--how_many_packets_to_process <= 0) break;
  }
  // flush all frames
  encode_frame(decoder->audio, encoder->audio,
               NULL, input_packet, encoder->format_context);

  encode_frame(decoder->video, encoder->video,
               NULL, input_packet, encoder->format_context);
  // should I do it for the audio stream too?

  logging("wrote %lu frames", counted_frames);
  av_write_trailer(encoder->format_context);

  logging("releasing all the resources");

  free_conversion_context(input_conversion_context);
  free_conversion_context(secondary_input_conversion_context);
  free_conversion_context(encoding_conversion);
  av_packet_free(&input_packet);
  av_frame_free(&inputFrame);
  av_frame_free(&outputFrame);
  av_frame_free(&secondary_input_frame);
  free_context(decoder);
  free_context(encoder);
  free_context(secondary_decoder);
  tearDownOpenGL();
  return 0;
}