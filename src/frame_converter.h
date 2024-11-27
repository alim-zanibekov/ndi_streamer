// Copyright 2022 Alim Zanibekov
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef FRAME_CONVERTER_H
#define FRAME_CONVERTER_H

#include <Processing.NDI.Lib.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>

typedef struct FrameConverterCtx {
    SwrContext *swr_context;
    struct SwsContext *sws_ctx;

    AVFrame *audio_frame;
    AVFrame *video_frame;

    int64_t frame_index;
    int64_t start_ts;

    char *error_str;
} FrameConverterCtx;

FrameConverterCtx *
new_frame_converter_ctx();

int
free_frame_converter_ctx(FrameConverterCtx **ctx);

void
fc_reset(FrameConverterCtx *);

AVFrame *
fc_ndi_video_frame_to_avframe(FrameConverterCtx *ctx, AVCodecContext *codec_ctx,
                              NDIlib_video_frame_v2_t *in_frame);

AVFrame *
fc_ndi_audio_frame_to_avframe(FrameConverterCtx *ctx, AVCodecContext *codec_ctx,
                              NDIlib_audio_frame_v2_t *in_frame);
#endif