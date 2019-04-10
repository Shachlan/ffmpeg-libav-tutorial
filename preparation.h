#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

typedef struct _TranscodeContext
{
    char *file_name;
    AVFormatContext *format_context;

    AVCodec *codec[2];
    AVStream *stream[2];
    AVCodecParameters *codec_parameters[2];
    AVCodecContext *codec_context[2];
    int video_stream_index;
    int audio_stream_index;
} TranscodeContext;

extern void logging(const char *fmt, ...);

extern int prepare_decoder(TranscodeContext *decoder_context);

extern int prepare_encoder(TranscodeContext *encoder_context, TranscodeContext *decoder_context);

#ifdef __cplusplus
}
#endif