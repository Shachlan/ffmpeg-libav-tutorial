#include "ffmpeg_wrappers.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
#include <stdbool.h>
#ifdef __cplusplus
}
#endif

void logging(const char *fmt, ...)
{
    va_list args;
    fprintf(stderr, "LOG: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

static TranscodeContext *make_context(char *file_name)
{
    TranscodeContext *context = (TranscodeContext *)calloc(1, sizeof(TranscodeContext));
    context->file_name = file_name;
    return context;
}

int prepare_components(FrameCodingComponents *prep)
{
    prep->codec = avcodec_find_decoder(prep->stream->codecpar->codec_id);
    prep->context = avcodec_alloc_context3(prep->codec);
    if (avcodec_parameters_to_context(prep->context, prep->stream->codecpar) < 0)
    {
        logging("failed to copy codec params to codec context");
        return -1;
    }

    if (avcodec_open2(prep->context, prep->codec, NULL) < 0)
    {
        logging("failed to open codec through avcodec_open2");
        return -1;
    }

    return 0;
}

int prepare_decoder(char *file_name, TranscodeContext **decoderRef)
{
    TranscodeContext *decoder = make_context(file_name);
    AVFormatContext *format_context = avformat_alloc_context();
    decoder->format_context = format_context;
    if (!format_context)
    {
        logging("ERROR could not allocate memory for Format Context");
        return -1;
    }

    if (avformat_open_input(&format_context, decoder->file_name, NULL, NULL) != 0)
    {
        logging("ERROR could not open the file");
        return -1;
    }

    if (avformat_find_stream_info(format_context, NULL) < 0)
    {
        logging("ERROR could not get the stream info");
        return -1;
    }

    int audio_stream_index = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    int video_stream_index = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    decoder->audio = (FrameCodingComponents *)calloc(1, sizeof(FrameCodingComponents));
    decoder->video = (FrameCodingComponents *)calloc(1, sizeof(FrameCodingComponents));
    decoder->audio->stream = format_context->streams[audio_stream_index];
    decoder->video->stream = format_context->streams[video_stream_index];
    if (prepare_components(decoder->audio) < 0)
    {
        logging("ERROR failed prepping audi context");
        return -1;
    }

    if (prepare_components(decoder->video) < 0)
    {
        logging("ERROR failed prepping video context");
        return -1;
    }

    *decoderRef = decoder;
    return 0;
}

static int prepare_video_encoder(TranscodeContext *encoder, TranscodeContext *decoder)
{
    encoder->video->stream = avformat_new_stream(encoder->format_context, NULL);
    encoder->video->codec = avcodec_find_encoder_by_name("libx264");

    if (!encoder->video->codec)
    {
        logging("could not find the proper codec");
        return -1;
    }

    encoder->video->context = avcodec_alloc_context3(encoder->video->codec);
    if (!encoder->video->context)
    {
        logging("could not allocated memory for codec context");
        return -1;
    }

    // how to free this?
    AVDictionary *encoder_options = NULL;

    AVCodecContext *encoder_codec_context = encoder->video->context;

    encoder_codec_context->height = decoder->video->context->height;
    encoder_codec_context->width = decoder->video->context->width;
    encoder_codec_context->sample_aspect_ratio = decoder->video->context->sample_aspect_ratio;

    if (encoder->video->codec->pix_fmts)
        encoder->video->context->pix_fmt = encoder->video->codec->pix_fmts[0];
    else
        encoder->video->context->pix_fmt = decoder->video->context->pix_fmt;

    if (avcodec_parameters_from_context(encoder->video->stream->codecpar, encoder->video->context) < 0)
    {
        logging("could not copy encoder parameters to output stream");
        return -1;
    }

    encoder->video->context->time_base = av_make_q(1, 30);
    encoder->video->context->framerate = av_make_q(30, 1);
    encoder->video->stream->time_base = encoder->video->context->time_base;
    encoder->video->context->ticks_per_frame = 1;

    if (avcodec_open2(encoder->video->context, encoder->video->codec, &encoder_options) < 0)
    {
        logging("could not open the codec");
        return -1;
    }

    return 0;
}

static int prepare_audio_copy(TranscodeContext *encoder, TranscodeContext *decoder)
{
    encoder->audio->stream = avformat_new_stream(encoder->format_context, NULL);
    encoder->audio->codec = avcodec_find_encoder(decoder->audio->codec->id);
    avcodec_parameters_copy(encoder->audio->stream->codecpar, decoder->audio->stream->codecpar);
    if (!encoder->audio->codec)
    {
        logging("could not find the proper codec");
        return -1;
    }

    encoder->audio->context = avcodec_alloc_context3(encoder->audio->codec);
    if (!encoder->audio->context)
    {
        logging("could not allocated memory for codec context");
        return -1;
    }

    if (avcodec_parameters_to_context(encoder->audio->context, encoder->audio->stream->codecpar) < 0)
    {
        logging("could not copy encoder parameters to context");
        return -1;
    }

    if (avcodec_parameters_from_context(encoder->audio->stream->codecpar, encoder->audio->context) < 0)
    {
        logging("could not copy encoder parameters to output stream");
        return -1;
    }

    encoder->audio->context->channel_layout = AV_CH_LAYOUT_STEREO;
    encoder->audio->context->channels = av_get_channel_layout_nb_channels(encoder->audio->context->channel_layout);

    if (avcodec_open2(encoder->audio->context, encoder->audio->codec, NULL) < 0)
    {
        logging("could not open the audio codec");
        return -1;
    }

    return 0;
}

int prepare_encoder(char *file_name, TranscodeContext *decoder, TranscodeContext **encoderRef)
{
    TranscodeContext *encoder = make_context(file_name);
    encoder->audio = (FrameCodingComponents *)calloc(1, sizeof(FrameCodingComponents));
    encoder->video = (FrameCodingComponents *)calloc(1, sizeof(FrameCodingComponents));
    avformat_alloc_output_context2(&encoder->format_context, NULL, NULL, encoder->file_name);
    if (!encoder->format_context)
    {
        logging("could not allocate memory for output format");
        return -1;
    }

    if (prepare_video_encoder(encoder, decoder))
    {
        logging("error while preparing video encoder");
        return -1;
    }

    if (prepare_audio_copy(encoder, decoder))
    {
        logging("error while preparing audio copy");
        return -1;
    }

    if (encoder->format_context->oformat->flags & AVFMT_GLOBALHEADER)
        encoder->format_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (!(encoder->format_context->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&encoder->format_context->pb, encoder->file_name, AVIO_FLAG_WRITE) < 0)
        {
            logging("could not open the output file");
            return -1;
        }
    }
    if (avformat_write_header(encoder->format_context, NULL) < 0)
    {
        logging("an error occurred when opening output file");
        return -1;
    }

    *encoderRef = encoder;
    return 0;
}

static int decode_single_packet(
    FrameCodingComponents *decoder,
    AVPacket *packet, AVFrame *inputFrame)
{
    AVCodecContext *codec_context = decoder->context;
    int response = avcodec_send_packet(codec_context, packet);

    if (response < 0)
    {
        logging("DECODER: Error while sending a packet to the decoder: %s", av_err2str(response));
        return response;
    }

    return avcodec_receive_frame(codec_context, inputFrame);
}

static int get_next_frame(TranscodeContext *decoder, AVPacket *packet, AVFrame *frame,
                          bool ignore_audio, bool ignore_video, AVMediaType *resulting_media)
{
    bool video_packet;
    int result = 0;
    while (result >= 0)
    {
        result = av_read_frame(decoder->format_context, packet);
        if (result == AVERROR_EOF)
        {
            break;
        }
        video_packet = packet->stream_index == decoder->video->stream->index;
        if ((ignore_audio && !video_packet) || (ignore_video && video_packet))
        {
            av_packet_unref(packet);
            continue;
        }

        FrameCodingComponents *components = video_packet ? decoder->video : decoder->audio;
        result = decode_single_packet(
            components, packet,
            frame);
        if (result == AVERROR(EAGAIN))
        {
            result = 0;
            av_packet_unref(packet);
            continue;
        }
        break;
    }
    *resulting_media = video_packet ? AVMEDIA_TYPE_VIDEO : AVMEDIA_TYPE_AUDIO;
    return result;
}

int get_next_frame(TranscodeContext *decoder, AVPacket *packet, AVFrame *frame, AVMediaType *resulting_media)
{

    return get_next_frame(decoder, packet, frame, false, false, resulting_media);
}

int get_next_video_frame(TranscodeContext *decoder, AVPacket *packet, AVFrame *frame)
{
    AVMediaType media;
    return get_next_frame(decoder, packet, frame,
                          true, false, &media);
}

int encode_frame(FrameCodingComponents *decoder,
                 FrameCodingComponents *encoder,
                 AVFrame *frame,
                 AVPacket *output_packet,
                 AVFormatContext *format_context)
{
    AVCodecContext *codec_context = encoder->context;

    int ret;
    //if (frame)
    // logging("Send frame %3" PRId64 "", frame->pts);
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
        output_packet->stream_index = encoder->stream->index;

        av_packet_rescale_ts(output_packet,
                             decoder->stream->time_base,
                             encoder->stream->time_base);

        /* mux encoded frame */
        ret = av_interleaved_write_frame(format_context, output_packet);

        if (ret != 0)
        {
            logging("Error %d while receiving a packet from the decoder: %s", ret, av_err2str(ret));
        }

        // logging("Write packet %d (size=%d)", output_packet->pts, output_packet->size);
    }
    av_packet_unref(output_packet);
    return 0;
}

static void free_components(FrameCodingComponents *components)
{
    avcodec_close(components->context);
    avcodec_free_context(&components->context);
    free(components);
}

void free_context(TranscodeContext *context)
{
    free_components(context->audio);
    free_components(context->video);
    avformat_close_input(&context->format_context);
    avformat_free_context(context->format_context);

    free(context);
}