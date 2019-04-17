
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#include <string>
using std::string;

struct TranscodingComponents {
    ~TranscodingComponents();
    AVCodec *codec;
    AVStream *stream;
    AVCodecContext *context;
    AVPacket *packet;
    AVFrame *frame;
};

struct DecodingComponents: TranscodingComponents {
    ~DecodingComponents();
    string file_name;
    AVFormatContext *format_context;
};

struct VideoDecodingComponents: DecodingComponents {
    ~VideoDecodingComponents();
    AVFrame *buffered_frame;
    long next_pts;
    long pts_increase_betweem_frames;
};

void logging(const char *fmt, ...);

int prepare_audio_decoder(string file_name, DecodingComponents **decoder);

int prepare_video_decoder(string file_name, AVRational expected_framerate, VideoDecodingComponents **decoder);

int prepare_encoder(string file_name, AVRational expected_framerate, DecodingComponents *decoder, DecodingComponents **encoder_ref);

int get_next_video_frame(VideoDecodingComponents *decoder);

int get_next_audio_frame(DecodingComponents *decoder);

int encode_frame(DecodingComponents *decoder,
                DecodingComponents *encoder);

int close_encoder(DecodingComponents *decoder, DecodingComponents *encoder);

void free_context(DecodingComponents *context);
