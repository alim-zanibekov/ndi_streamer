// Copyright 2022 Alim Zanibekov
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef FFMPEG_OUTPUT_H
#define FFMPEG_OUTPUT_H

#include <libavcodec/avcodec.h>

typedef struct FFmpegOutputCtx {
    struct AVFormatContext *o_ctx;
    struct AVCodecContext *audio_codec_ctx;
    struct AVCodecContext *video_codec_ctx;
    int audio_stream_index;
    int video_stream_index;
    const char *output;
    char *error_str;
} FFmpegOutputCtx;

FFmpegOutputCtx *
new_ffmpeg_output_ctx();

int
free_ffmpeg_output_ctx(FFmpegOutputCtx **ctx);

int
ffmpeg_output_init(FFmpegOutputCtx *ctx, const char *format,
                   const char *output);

void
ffmpeg_output_close(FFmpegOutputCtx *ctx);

void
ffmpeg_output_close_codecs(FFmpegOutputCtx *ctx);

int
ffmpeg_output_write_header(FFmpegOutputCtx *ctx, AVDictionary **av_opts);

int
ffmpeg_output_setup_video(FFmpegOutputCtx *ctx, const char *encoder_name,
                          int width, int height, AVRational framerate,
                          int64_t bitrate);

int
ffmpeg_output_setup_audio(FFmpegOutputCtx *ctx, char *encoder_name,
                          int64_t bitrate);
int
ffmpeg_output_send_video_frame(FFmpegOutputCtx *ctx, AVFrame *frame);

int
ffmpeg_output_send_audio_frame(FFmpegOutputCtx *ctx, AVFrame *frame);

#endif
