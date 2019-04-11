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

extern TranscodeContext *make_context(char *file_name);

extern int prepare_decoder(TranscodeContext *decoder);

extern int prepare_encoder(TranscodeContext *encoder, TranscodeContext *decoder);

extern int get_next_video_frame(TranscodeContext *decoder, AVPacket *packet, AVFrame *frame);

extern int get_next_frame(TranscodeContext *decoder, AVPacket *packet, AVFrame *frame, AVMediaType *resulting_media);

extern int encode_frame(FrameCodingComponents *decoder,
                        FrameCodingComponents *encoder,
                        AVFrame *frame,
                        AVPacket *output_packet,
                        AVFormatContext *format_context);

void free_context(TranscodeContext *context);

#ifdef __cplusplus
}
#endif