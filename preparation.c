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

int prepare_decoder(TranscodeContext *decoder_context)
{
    decoder_context->format_context = avformat_alloc_context();
    if (!decoder_context->format_context)
    {
        logging("ERROR could not allocate memory for Format Context");
        return -1;
    }

    if (avformat_open_input(&decoder_context->format_context, decoder_context->file_name, NULL, NULL) != 0)
    {
        logging("ERROR could not open the file");
        return -1;
    }

    if (avformat_find_stream_info(decoder_context->format_context, NULL) < 0)
    {
        logging("ERROR could not get the stream info");
        return -1;
    }

    for (int i = 0; i < decoder_context->format_context->nb_streams; i++)
    {
        if (decoder_context->format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            decoder_context->video_stream_index = i;
            logging("Video");
        }
        else if (decoder_context->format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            decoder_context->audio_stream_index = i;
            logging("Audio");
        }
        decoder_context->codec_parameters[i] = decoder_context->format_context->streams[i]->codecpar;
        decoder_context->stream[i] = decoder_context->format_context->streams[i];

        logging("\tAVStream->time_base before open coded %d/%d", decoder_context->stream[i]->time_base.num, decoder_context->stream[i]->time_base.den);
        logging("\tAVStream->r_frame_rate before open coded %d/%d", decoder_context->stream[i]->r_frame_rate.num, decoder_context->stream[i]->r_frame_rate.den);
        logging("\tAVStream->start_time %" PRId64, decoder_context->stream[i]->start_time);
        logging("\tAVStream->duration %" PRId64, decoder_context->stream[i]->duration);

        decoder_context->codec[i] = avcodec_find_decoder(decoder_context->codec_parameters[i]->codec_id);
        if (!decoder_context->codec[i])
        {
            logging("ERROR unsupported codec!");
            return -1;
        }

        decoder_context->codec_context[i] = avcodec_alloc_context3(decoder_context->codec[i]);
        if (!decoder_context->codec[i])
        {
            logging("failed to allocated memory for AVCodecContext");
            return -1;
        }

        if (avcodec_parameters_to_context(decoder_context->codec_context[i], decoder_context->codec_parameters[i]) < 0)
        {
            logging("failed to copy codec params to codec context");
            return -1;
        }

        if (avcodec_open2(decoder_context->codec_context[i], decoder_context->codec[i], NULL) < 0)
        {
            logging("failed to open codec through avcodec_open2");
            return -1;
        }
    }
    //printf("first format: %s\n", av_get_pix_fmt_name(decoder_context->codec_context[0]->pix_fmt));
    //printf("second format: %s\n", av_get_pix_fmt_name(decoder_context->codec_context[1]->pix_fmt));
    return 0;
}

static int prepare_video_encoder(TranscodeContext *encoder_context, TranscodeContext *decoder_context)
{
    int index = decoder_context->video_stream_index;
    encoder_context->stream[index] = avformat_new_stream(encoder_context->format_context, NULL);
    encoder_context->codec[index] = avcodec_find_encoder_by_name("libx264");

    if (!encoder_context->codec[index])
    {
        logging("could not find the proper codec");
        return -1;
    }

    encoder_context->codec_context[index] = avcodec_alloc_context3(encoder_context->codec[index]);
    if (!encoder_context->codec_context[index])
    {
        logging("could not allocated memory for codec context");
        return -1;
    }

    // how to free this?
    AVDictionary *encoder_options = NULL;
    av_opt_set(&encoder_options, "keyint", "60", 0);
    av_opt_set(&encoder_options, "min-keyint", "60", 0);
    av_opt_set(&encoder_options, "no-scenecut", "1", 0);

    AVCodecContext *encoder_codec_context = encoder_context->codec_context[index];
    av_opt_set(encoder_codec_context->priv_data, "keyint", "60", 0);
    av_opt_set(encoder_codec_context->priv_data, "min-keyint", "60", 0);
    av_opt_set(encoder_codec_context->priv_data, "no-scenecut", "1", 0);

    encoder_codec_context->height = decoder_context->codec_context[index]->height;
    encoder_codec_context->width = decoder_context->codec_context[index]->width;
    encoder_codec_context->sample_aspect_ratio = decoder_context->codec_context[index]->sample_aspect_ratio;

    if (encoder_context->codec[index]->pix_fmts)
        encoder_context->codec_context[index]->pix_fmt = encoder_context->codec[index]->pix_fmts[0];
    else
        encoder_context->codec_context[index]->pix_fmt = decoder_context->codec_context[index]->pix_fmt;

    encoder_context->codec_context[index]->time_base = decoder_context->stream[index]->time_base;

    if (avcodec_parameters_from_context(encoder_context->stream[index]->codecpar, encoder_context->codec_context[index]) < 0)
    {
        logging("could not copy encoder parameters to output stream");
        return -1;
    }

    if (avcodec_open2(encoder_context->codec_context[index], encoder_context->codec[index], &encoder_options) < 0)
    {
        logging("could not open the codec");
        return -1;
    }

    encoder_context->stream[index]->time_base = encoder_context->codec_context[index]->time_base;
    return 0;
}

static int prepare_audio_copy(TranscodeContext *encoder_context, TranscodeContext *decoder_context)
{
    int index = decoder_context->audio_stream_index;
    encoder_context->stream[index] = avformat_new_stream(encoder_context->format_context, NULL);
    encoder_context->codec[index] = avcodec_find_encoder(decoder_context->codec[index]->id);
    avcodec_parameters_copy(encoder_context->stream[index]->codecpar, decoder_context->stream[index]->codecpar);
    if (!encoder_context->codec[index])
    {
        logging("could not find the proper codec");
        return -1;
    }

    encoder_context->codec_context[index] = avcodec_alloc_context3(encoder_context->codec[index]);
    if (!encoder_context->codec_context[index])
    {
        logging("could not allocated memory for codec context");
        return -1;
    }

    if (avcodec_parameters_to_context(encoder_context->codec_context[index], encoder_context->stream[index]->codecpar) < 0)
    {
        logging("could not copy encoder parameters to context");
        return -1;
    }

    if (avcodec_parameters_from_context(encoder_context->stream[index]->codecpar, encoder_context->codec_context[index]) < 0)
    {
        logging("could not copy encoder parameters to output stream");
        return -1;
    }

    if (avcodec_open2(encoder_context->codec_context[index], encoder_context->codec[index], NULL) < 0)
    {
        logging("could not open the codec");
        return -1;
    }

    return 0;
}

int prepare_encoder(TranscodeContext *encoder_context, TranscodeContext *decoder_context)
{
    avformat_alloc_output_context2(&encoder_context->format_context, NULL, NULL, encoder_context->file_name);
    if (!encoder_context->format_context)
    {
        logging("could not allocate memory for output format");
        return -1;
    }

    if (prepare_video_encoder(encoder_context, decoder_context))
    {
        logging("error while preparing video encoder");
        return -1;
    }

    if (prepare_audio_copy(encoder_context, decoder_context))
    {
        logging("error while preparing audio copy");
        return -1;
    }

    if (encoder_context->format_context->oformat->flags & AVFMT_GLOBALHEADER)
        encoder_context->format_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (!(encoder_context->format_context->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&encoder_context->format_context->pb, encoder_context->file_name, AVIO_FLAG_WRITE) < 0)
        {
            logging("could not open the output file");
            return -1;
        }
    }
    if (avformat_write_header(encoder_context->format_context, NULL) < 0)
    {
        logging("an error occurred when opening output file");
        return -1;
    }

    return 0;
}