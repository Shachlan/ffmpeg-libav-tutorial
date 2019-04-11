#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

typedef struct _FrameCodingComponents
{
    AVCodec *codec;
    AVStream *stream;
    AVCodecContext *context;
} FrameCodingComponents;

typedef struct _TranscodeContext
{
    char *file_name;
    AVFormatContext *format_context;

    FrameCodingComponents *video;
    FrameCodingComponents *audio;
} TranscodeContext;

extern void logging(const char *fmt, ...);

extern int prepare_decoder(TranscodeContext *decoder_context);

extern int prepare_encoder(TranscodeContext *encoder_context, TranscodeContext *decoder_context);

#ifdef __cplusplus
}
#endif