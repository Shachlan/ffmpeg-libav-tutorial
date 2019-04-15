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
    AVFrame *buffered_frame;
    long next_pts;
    long pts_increase_betweem_frames;
    char *name;
} FrameCodingComponents;

typedef struct _TranscodeContext
{
    char *file_name;
    AVFormatContext *format_context;

    FrameCodingComponents *video;
    FrameCodingComponents *audio;
} TranscodeContext;

extern void logging(const char *fmt, ...);

extern int prepare_decoder(char *file_name, AVRational expected_framerate, TranscodeContext **decoder);

extern int prepare_encoder(char *file_name, AVRational expected_framerate, TranscodeContext *decoder, TranscodeContext **encoderRef);

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