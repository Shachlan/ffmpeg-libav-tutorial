#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
extern "C" {
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include "./rationalExtensions.h"
}
#include "./video_conversion_utilities.hpp"
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
  printf("start convert frame\n");
  convert_from_frame(inputFrame, decoding_conversion);
  printf("invert frame\n");
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
  printf("blend frames\n");
  loadTexture(texture1ID, inputFrame->width, inputFrame->height, decoding_conversion->rgb_buffer[0]);
  loadTexture(texture2ID, secondary_input_frame->width, secondary_input_frame->height, secondary_decoding_conversion->rgb_buffer[0]);
  blendFrames(texture1ID, texture2ID);
  getCurrentResults(outputFrame->width, outputFrame->height, encoding_conversion->rgb_buffer[0]);
  //printf("rescale\n");
  convert_to_frame(outputFrame, encoding_conversion);
}

int main(int argc, char *argv[])
{
  logging("start");
  VideoDecodingComponents *decoder;
  VideoDecodingComponents *secondary_decoder;
  DecodingComponents *encoder;
  AVRational expected_framerate = av_make_q(30, 1);

logging("video decoder");
  if (prepare_video_decoder(argv[1], expected_framerate, &decoder))
  {
    logging("error while preparing input");
    return -1;
  }

logging("secondary decoder");
  if (prepare_video_decoder(argv[2], expected_framerate, &secondary_decoder))
  {
    logging("error while preparing secondary input");
    return -1;
  }

logging("encoder");
  if (prepare_encoder(argv[3], expected_framerate, decoder, &encoder))
  {
    logging("error while preparing output");
    return -1;
  }

  logging("next");
  float blendRatio = strtof(argv[4], NULL);
  printf("blend ratio: %f\n", blendRatio);
  int wait = atoi(argv[5]);
  int length = atoi(argv[6]);

  AVCodecContext *video_encoding_context = encoder->context;
  AVCodecContext *secondary_video_context = secondary_decoder->context;
  int height = video_encoding_context->height;
  int width = video_encoding_context->width;
  setupOpenGL(width, height, blendRatio, NULL);
  uint32_t tex1 = createTexture();
  uint32_t tex2 = createTexture();
  ConversionContext *input_conversion_context = create_decoding_conversion_context(decoder->context);
  ConversionContext *secondary_input_conversion_context = create_decoding_conversion_context(secondary_video_context);
  ConversionContext *encoding_conversion = create_encoding_conversion_context(video_encoding_context);

  AVStream *secondary_video_stream = secondary_decoder->stream;

  AVRational time_base = decoder->stream->time_base;
  AVRational secondary_time_base = secondary_video_stream->time_base;
  double duration_in_seconds = av_q2d(multiply_by_int(secondary_time_base, secondary_video_stream->duration));
  double maxTime = (length > duration_in_seconds ? duration_in_seconds : length) + wait;
  printf("duration: %lf, max time: %lf, wait: %d\n", duration_in_seconds, maxTime, wait);
  int response;

  long counted_frames = 0;
  long blended_frames = 0;
  long inverted_frames = 0;
  long audio_frames = 0;
  while (get_next_video_frame(decoder) >= 0)
  {
    // bool video_frame = media_type == AVMEDIA_TYPE_VIDEO;
    // if (!video_frame)
    // {
    //   //logging("audio frame %lu", input_packet->pts);
    //   av_packet_unref(input_packet);
    //   response = encode_frame(components, encoder->audio, inputFrame, input_packet, encoder->format_context);
    //   if (response < 0)
    //   {
    //     logging("DECODER: Error while receiving an audio frame from the decoder: %s", av_err2str(response));
    //     return response;
    //   }
    //   audio_frames++;
    //   av_frame_unref(inputFrame);
    //   continue;
    // }
    counted_frames++;

    AVRational current_pos = multiply_by_int(time_base, decoder->packet->pts);
    double point_in_time = av_q2d(current_pos);
    logging("point in time %f", point_in_time);
    if (point_in_time < wait || point_in_time > maxTime)
    {
      inverted_frames++;
      invert_single_frame(decoder->frame, encoder->frame, tex1,
                          input_conversion_context, encoding_conversion);
      encode_frame(decoder, encoder);
      //av_frame_unref(inputFrame);
      continue;
    }

    response = get_next_video_frame(secondary_decoder);
    if (response < 0 && response != AVERROR_EOF)
    {
      logging("DECODER: Error while receiving a frame from the secondary decoder: %d %s", response, av_err2str(response));
      return response;
    }
    else if (response == AVERROR_EOF)
    {
      inverted_frames++;
      invert_single_frame(decoder->frame, encoder->frame, tex1,
                          input_conversion_context, encoding_conversion);
      encode_frame(decoder, encoder);
      continue;
    }

    blended_frames++;
    blend_frames(decoder->frame, secondary_decoder->frame, encoder->frame, tex1, tex2,
                 input_conversion_context, secondary_input_conversion_context, encoding_conversion);
      encode_frame(decoder, encoder);
    //if (--how_many_packets_to_process <= 0) break;
  }
  // // flush all frames
  // encode_frame(decoder->audio, encoder->audio,
  //              NULL, input_packet, encoder->format_context);

  close_encoder(decoder, encoder);
  // should I do it for the audio stream too?

  logging("wrote %lu frames. %lu blended, %lu inverted, %lu audio", counted_frames, blended_frames, inverted_frames, audio_frames);
  av_write_trailer(encoder->format_context);

  logging("releasing all the resources");

  free_conversion_context(input_conversion_context);
  free_conversion_context(secondary_input_conversion_context);
  free_conversion_context(encoding_conversion);
  free_context(decoder);
  free_context(encoder);
  free_context(secondary_decoder);
  tearDownOpenGL();
  return 0;
}