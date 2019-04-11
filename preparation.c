#include "preparation.h"

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

int prepare_decoder(TranscodeContext *decoder)
{
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
    av_opt_set(&encoder_options, "keyint", "60", 0);
    av_opt_set(&encoder_options, "min-keyint", "60", 0);
    av_opt_set(&encoder_options, "no-scenecut", "1", 0);

    AVCodecContext *encoder_codec_context = encoder->video->context;
    av_opt_set(encoder_codec_context->priv_data, "keyint", "60", 0);
    av_opt_set(encoder_codec_context->priv_data, "min-keyint", "60", 0);
    av_opt_set(encoder_codec_context->priv_data, "no-scenecut", "1", 0);

    encoder_codec_context->height = decoder->video->context->height;
    encoder_codec_context->width = decoder->video->context->width;
    encoder_codec_context->sample_aspect_ratio = decoder->video->context->sample_aspect_ratio;

    if (encoder->video->codec->pix_fmts)
        encoder->video->context->pix_fmt = encoder->video->codec->pix_fmts[0];
    else
        encoder->video->context->pix_fmt = decoder->video->context->pix_fmt;

    encoder->video->context->time_base = decoder->video->stream->time_base;

    if (avcodec_parameters_from_context(encoder->video->stream->codecpar, encoder->video->context) < 0)
    {
        logging("could not copy encoder parameters to output stream");
        return -1;
    }

    if (avcodec_open2(encoder->video->context, encoder->video->codec, &encoder_options) < 0)
    {
        logging("could not open the codec");
        return -1;
    }

    encoder->video->stream->time_base = encoder->video->context->time_base;
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

    if (avcodec_open2(encoder->audio->context, encoder->audio->codec, NULL) < 0)
    {
        logging("could not open the codec");
        return -1;
    }

    return 0;
}

int prepare_encoder(TranscodeContext *encoder, TranscodeContext *decoder)
{
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

    return 0;
}