#include "WREDecoders.hpp"

#include "WREFFmpegTranscoding.hpp"
#include "WREVideoFormatConverter.hpp"

/// Components needed in order to decode media from a given file using FFmpeg. This struct cleanly
/// wraps the frame decoding process.
struct WREDecodingComponents : WRETranscodingComponents {
  /// Factory method for the creation of audio decoders. Will an audio decoder for the file at \c
  /// file_name. Will return \c nullptr if file opening failed.
  static WREDecodingComponents *get_audio_decoder(string file_name);

  virtual ~WREDecodingComponents();

  /// Decodes the next frame in the file into the internal \c frame.
  virtual int decode_next_frame();
  AVFormatContext *format_context;
};

/// Components needed in order to decode video from a given file using FFmpeg. This struct cleanly
/// wraps the frame decoding process, including handling framerate differences between the source
/// file and the framerate expected by the target video target.
struct WREVideoDecodingComponents : WREDecodingComponents {
  /// Factory method for the creation of audio decoders. Will an audio decoder for the file at \c
  /// file_name. Will return \c nullptr if file opening failed. \c expected_framerate will determine
  /// how many frames will be decoded per second of video, using a best-guess estimate that will
  /// drop or add frames, if the original video's framerate is different from \c expected_framerate.
  static WREVideoDecodingComponents *get_video_decoder(string file_name,
                                                       AVRational expected_framerate);

  ~WREVideoDecodingComponents();

  /// Decodes the next frame in the file into the internal \c frame.
  int decode_next_frame() override;

private:
  AVFrame *buffered_frame;
  long next_pts;
  long pts_increase_betweem_frames;
};

WREDecodingComponents::~WREDecodingComponents() {
  avformat_close_input(&format_context);
  avformat_free_context(format_context);
}

WREVideoDecodingComponents::~WREVideoDecodingComponents() {
  av_frame_free(&buffered_frame);
}

int prepare_decoding_components(WREDecodingComponents *decoder, string file_name,
                                AVMediaType media) {
  AVFormatContext *format_context = avformat_alloc_context();
  decoder->format_context = format_context;
  if (!format_context) {
    log_error("ERROR could not allocate memory for Format Context");
    return -1;
  }

  if (avformat_open_input(&format_context, file_name.c_str(), NULL, NULL) != 0) {
    log_error("ERROR could not open %s", file_name.c_str());
    return -1;
  }

  if (avformat_find_stream_info(format_context, NULL) < 0) {
    log_error("ERROR could not get the stream info");
    return -1;
  }

  AVStream *stream =
      format_context->streams[av_find_best_stream(format_context, media, -1, -1, NULL, 0)];

  decoder->stream = stream;
  decoder->codec = avcodec_find_decoder(stream->codecpar->codec_id);
  decoder->context = avcodec_alloc_context3(decoder->codec);
  if (avcodec_parameters_to_context(decoder->context, stream->codecpar) < 0) {
    log_error("failed to copy codec params to codec context");
    return -1;
  }

  if (avcodec_open2(decoder->context, decoder->codec, NULL) < 0) {
    log_error("failed to open codec through avcodec_open2");
    return -1;
  }
  decoder->packet = av_packet_alloc();
  decoder->frame = av_frame_alloc();

  return 0;
}

WREDecodingComponents *WREDecodingComponents::get_audio_decoder(string file_name) {
  WREDecodingComponents *decoder = new WREDecodingComponents();
  if (prepare_decoding_components(decoder, file_name, AVMEDIA_TYPE_AUDIO) != 0) {
    delete (decoder);
    return nullptr;
  }
  return decoder;
}

WREVideoDecodingComponents *WREVideoDecodingComponents::get_video_decoder(
    string file_name, AVRational expected_framerate) {
  WREVideoDecodingComponents *decoder = new WREVideoDecodingComponents();
  if (prepare_decoding_components(decoder, file_name, AVMEDIA_TYPE_VIDEO) != 0) {
    delete (decoder);
    return nullptr;
  }
  decoder->buffered_frame = av_frame_alloc();
  decoder->buffered_frame->pts = INT64_MIN;
  decoder->pts_increase_betweem_frames =
      (long)av_q2d(av_inv_q(av_mul_q(decoder->stream->time_base, expected_framerate)));
  decoder->next_pts = 0;
  return decoder;
}

static int decode_single_packet(WREDecodingComponents *decoder) {
  AVCodecContext *codec_context = decoder->context;
  int response = avcodec_send_packet(codec_context, decoder->packet);

  if (response < 0) {
    log_error("DECODER: Error while sending a packet to the decoder: %s", av_err2str(response));
    return response;
  }

  return avcodec_receive_frame(codec_context, decoder->frame);
}

int WREVideoDecodingComponents::decode_next_frame() {
  int result = 0;
  if (this->buffered_frame->pts >= this->next_pts) {
    av_frame_copy(this->frame, this->buffered_frame);
    av_frame_copy_props(this->frame, this->buffered_frame);
    this->frame->pts = this->next_pts;
    this->next_pts += this->pts_increase_betweem_frames;
    return 0;
  }

  while (result >= 0) {
    av_packet_unref(this->packet);
    result = av_read_frame(this->format_context, this->packet);
    if (result == AVERROR_EOF) {
      break;
    }
    if (this->packet->stream_index != this->stream->index) {
      continue;
    }

    av_frame_unref(this->frame);
    result = decode_single_packet(this);
    if (result == AVERROR(EAGAIN)) {
      result = 0;
      continue;
    }

    av_frame_unref(this->buffered_frame);
    av_frame_copy(this->buffered_frame, this->frame);
    av_frame_copy_props(this->buffered_frame, this->frame);
    return this->decode_next_frame();
  }
  return result;
}

int WREDecodingComponents::decode_next_frame() {
  int result = 0;
  while (result >= 0) {
    result = av_read_frame(this->format_context, this->packet);
    if (result == AVERROR_EOF) {
      break;
    }
    if (this->packet->stream_index != this->stream->index) {
      av_packet_unref(this->packet);
      continue;
    }

    av_frame_unref(this->frame);
    result = decode_single_packet(this);
    if (result == AVERROR(EAGAIN)) {
      result = 0;
      av_packet_unref(this->packet);
      continue;
    }
    break;
  }
  return result;
}

long WREDecoder::get_current_timestamp() {
  return internal_decoder->frame->pts;
}

double WREDecoder::get_time_base() {
  return av_q2d(internal_decoder->stream->time_base);
}

double WREDecoder::get_duration() {
  auto stream = internal_decoder->stream;
  AVRational secondary_time_base = stream->time_base;
  return av_q2d(av_mul_q(secondary_time_base, av_make_q(stream->duration, 1)));
}

WREDecoder::~WREDecoder() {
  delete (internal_decoder);
}

WREAudioDecoder::WREAudioDecoder(string file_name) {
  internal_decoder = WREDecodingComponents::get_audio_decoder(file_name);
}

WRETranscodingComponents *WREAudioDecoder::get_transcoding_components() {
  return internal_decoder;
}

int WREAudioDecoder::decode_next_frame() {
  return internal_decoder->decode_next_frame();
}

WREVideoDecoder::WREVideoDecoder(string file_name, double expected_framerate) {
  video_decoder =
      WREVideoDecodingComponents::get_video_decoder(file_name, av_d2q(expected_framerate, 300));
  internal_decoder = video_decoder;
  video_conversion_context =
      WREVideoFormatConverter::create_decoding_conversion_context(video_decoder->context);
}

WREVideoDecoder::~WREVideoDecoder() {
  delete (video_conversion_context);
}

int WREVideoDecoder::get_width() {
  return internal_decoder->context->width;
}

int WREVideoDecoder::get_height() {
  return internal_decoder->context->height;
}

uint8_t *WREVideoDecoder::get_rgb_buffer() {
  return this->video_conversion_context->get_rgb_buffer();
}

int WREVideoDecoder::decode_next_frame() {
  int result = video_decoder->decode_next_frame();
  if (result == 0) {
    this->video_conversion_context->convert_from_frame(this->video_decoder->frame);
  }
  return result;
}
