// Copyright 2022 Alim Zanibekov
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "ffmpeg_output.h"

#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>

#include "common.h"

FFmpegOutputCtx *
new_ffmpeg_output_ctx()
{
    FFmpegOutputCtx *ctx = malloc(sizeof(FFmpegOutputCtx));
    memset(ctx, 0, sizeof(FFmpegOutputCtx));
    ctx->error_str = malloc(AV_ERROR_MAX_STRING_SIZE + 100);
    return ctx;
}

int
free_ffmpeg_output_ctx(FFmpegOutputCtx **ctx)
{
    if ((*ctx)->audio_codec_ctx)
        avcodec_free_context(&(*ctx)->audio_codec_ctx);
    if ((*ctx)->video_codec_ctx)
        avcodec_free_context(&(*ctx)->video_codec_ctx);
    if ((*ctx)->o_ctx)
        avformat_close_input(&(*ctx)->o_ctx);

    free(*ctx);
    *ctx = NULL;
    return 0;
}

int
ffmpeg_output_init(FFmpegOutputCtx *ctx, const char *format, const char *output)
{
    int ret = avformat_alloc_output_context2(&ctx->o_ctx, NULL, format, output);
    if (ret < 0) {
        av_error_fmt(ctx->error_str,
                     "could not allocate output format context!", ret);
    }
    else if (!(ctx->o_ctx->oformat->flags & (int)AVFMT_NOFILE)) {

        ret = avio_open2(&ctx->o_ctx->pb, output, AVIO_FLAG_WRITE, NULL, NULL);
        if (ret < 0) {
            av_error_fmt(ctx->error_str, "could not open output IO context!",
                         ret);
            avformat_close_input(&ctx->o_ctx);
        }
    }

    ctx->output = output;
    return ret;
}

void
ffmpeg_output_close(FFmpegOutputCtx *ctx)
{
    if (ctx->audio_codec_ctx)
        avcodec_free_context(&ctx->audio_codec_ctx);
    if (ctx->video_codec_ctx)
        avcodec_free_context(&ctx->video_codec_ctx);
    if (ctx->o_ctx)
        avformat_close_input(&ctx->o_ctx);

    ctx->output = NULL;
}

void
ffmpeg_output_close_codecs(FFmpegOutputCtx *ctx)
{
    if (ctx->audio_codec_ctx)
        avcodec_free_context(&ctx->audio_codec_ctx);
    if (ctx->video_codec_ctx)
        avcodec_free_context(&ctx->video_codec_ctx);
}

int
ffmpeg_output_setup_video(FFmpegOutputCtx *ctx, const char *encoder_name,
                          const int width, const int height,
                          const AVRational framerate, const int64_t bitrate)
{
    const AVCodec *codec = avcodec_find_encoder_by_name(encoder_name);
    if (!codec) {
        sprintf(ctx->error_str, "%s", "could not find video codec");
        return -1;
    }
    AVStream *stream = avformat_new_stream(ctx->o_ctx, codec);
    if (!stream) {
        sprintf(ctx->error_str, "%s", "could not create video stream");
        return -1;
    }
    ctx->video_stream_index = (int)(ctx->o_ctx->nb_streams) - 1;
    AVCodecContext *c_ctx = avcodec_alloc_context3(codec);
    if (!c_ctx) {
        sprintf(ctx->error_str, "%s", "could not allocate video codec context");
        return -1;
    }
    c_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    c_ctx->time_base.num = 1;
    c_ctx->time_base.den = AV_TIME_BASE;
    c_ctx->width = width;
    c_ctx->height = height;
    c_ctx->framerate = framerate;
    c_ctx->bit_rate = bitrate;
    c_ctx->gop_size = 12;

    if (ctx->o_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        c_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    int ret;
    AVDictionary *codec_options = NULL;

    //    av_dict_set(&codec_options, "deadline", "realtime", 0);

    if (strcmp(encoder_name, "libx264") == 0) {
        av_dict_set(&codec_options, "preset", "veryfast", 0);
    }

    if (codec->id == AV_CODEC_ID_H264) {
        // av_dict_set(&codec_options, "profile", "baseline", 0);
        // av_dict_set(&codec_options, "level", "1.3", 0);
    }

    if ((ret = avcodec_open2(c_ctx, codec, &codec_options)) < 0) {
        av_error_fmt(ctx->error_str, "could not open video codec!", ret);
        avcodec_free_context(&c_ctx);
    }
    else if ((ret = avcodec_parameters_from_context(stream->codecpar, c_ctx))
             < 0) {
        av_error_fmt(ctx->error_str,
                     "could not initialize video stream codec parameters!",
                     ret);
        avcodec_free_context(&c_ctx);
    }
    else {
        ctx->video_codec_ctx = c_ctx;
    }

    av_dict_free(&codec_options);
    return ret;
}

int
ffmpeg_output_setup_audio(FFmpegOutputCtx *ctx, char *encoder_name,
                          int64_t bitrate)
{
    const AVCodec *codec = avcodec_find_encoder_by_name(encoder_name);
    if (!codec) {
        sprintf(ctx->error_str, "%s", "could not find audio codec");
        return -1;
    }
    AVStream *stream = avformat_new_stream(ctx->o_ctx, codec);
    if (!stream) {
        sprintf(ctx->error_str, "%s", "could not create audio stream");
        return -1;
    }
    ctx->audio_stream_index = (int)(ctx->o_ctx->nb_streams) - 1;

    AVCodecContext *c_ctx = avcodec_alloc_context3(codec);
    if (!c_ctx) {
        sprintf(ctx->error_str, "%s", "could not allocate audio codec context");
        return -1;
    }

    c_ctx->time_base.num = 1;
    c_ctx->time_base.den = AV_TIME_BASE;
    c_ctx->codec_type = AVMEDIA_TYPE_AUDIO;

    const int *supported_samplerates = NULL;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(61, 13, 100)
    supported_samplerates = codec->supported_samplerates;
#else
    avcodec_get_supported_config(c_ctx, codec, AV_CODEC_CONFIG_SAMPLE_RATE, 0,
                                 (const void **)&supported_samplerates, NULL);
#endif

    if (supported_samplerates != NULL) {
        int s_rate = 0;
        for (int i = 0; supported_samplerates[i] != 0; ++i) {
            if (supported_samplerates[i] == 48000) {
                s_rate = 48000;
                break;
            }
        }
        if (s_rate == 0) {
            s_rate = supported_samplerates[0];
        }
        c_ctx->sample_rate = s_rate;
    }
    else {
        c_ctx->sample_rate = 48000;
    }

    const AVChannelLayout *ch_layouts = NULL;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(61, 13, 100)
    ch_layouts = codec->ch_layouts;
#else
    avcodec_get_supported_config(c_ctx, codec, AV_CODEC_CONFIG_CHANNEL_LAYOUT,
                                 0, (const void **)&ch_layouts, NULL);
#endif

    if (ch_layouts != NULL) {
        AVChannelLayout ch_layout = {};
        for (int i = 0; ch_layouts[i].nb_channels != 0; ++i) {
            ch_layout = ch_layouts[i];
            if (ch_layout.nb_channels == 2
                && ch_layout.u.mask == AV_CH_LAYOUT_STEREO)
                break;
        }
        c_ctx->ch_layout = ch_layout;
    }
    else {
        AVChannelLayout ch_layout = AV_CHANNEL_LAYOUT_STEREO;
        c_ctx->ch_layout = ch_layout;
    }

    const enum AVSampleFormat *sample_fmts = NULL;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(61, 13, 100)
    sample_fmts = codec->sample_fmts;
#else
    avcodec_get_supported_config(c_ctx, codec, AV_CODEC_CONFIG_SAMPLE_FORMAT, 0,
                                 (const void **)&sample_fmts, NULL);
#endif

    if (sample_fmts != NULL) {
        enum AVSampleFormat sample_fmt = 0;
        for (int i = 0; sample_fmts[i] != 0; ++i) {
            sample_fmt = sample_fmts[i];
            if (sample_fmt == AV_SAMPLE_FMT_FLTP)
                break;
        }
        if (sample_fmt != AV_SAMPLE_FMT_FLTP) {
            sample_fmt = sample_fmts[0];
        }
        c_ctx->sample_fmt = sample_fmt;
    }

    c_ctx->bit_rate = bitrate;

    AVDictionary *codec_options = NULL;

    // av_dict_set(&codec_options, "frame_duration", "30", 0);

    if (ctx->o_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        c_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    int ret;
    if ((ret = avcodec_open2(c_ctx, codec, &codec_options)) < 0) {
        av_error_fmt(ctx->error_str, "could not open audio codec!", ret);
        avcodec_free_context(&c_ctx);
    }
    else if ((ret = avcodec_parameters_from_context(stream->codecpar, c_ctx))
             < 0) {
        av_error_fmt(ctx->error_str,
                     "could not initialize audio stream codec parameters!",
                     ret);
        avcodec_free_context(&c_ctx);
    }
    else {
        ctx->audio_codec_ctx = c_ctx;
    }
    av_dict_free(&codec_options);
    return ret;
}

int
ffmpeg_output_write_header(FFmpegOutputCtx *ctx, AVDictionary **av_opts)
{
    ctx->o_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    av_dump_format(ctx->o_ctx, 0, ctx->output, 1);

    int ret = avformat_write_header(ctx->o_ctx, av_opts);
    if (ret < 0) {
        av_error_fmt(ctx->error_str, "could not write header!", ret);
    }
    return ret;
}

int
send_packets(const FFmpegOutputCtx *ctx, AVCodecContext *codec_context,
             int stream_index);

int
ffmpeg_output_send_video_frame(FFmpegOutputCtx *ctx, AVFrame *frame)
{
    int ret = avcodec_send_frame(ctx->video_codec_ctx, frame);
    av_frame_unref(frame);
    if (ret < 0) {
        av_error_fmt(ctx->error_str,
                     "error sending frame to video codec context!", ret);
        return ret;
    }

    return send_packets(ctx, ctx->video_codec_ctx, ctx->video_stream_index);
}

int
ffmpeg_output_send_audio_frame(FFmpegOutputCtx *ctx, AVFrame *frame)
{
    int ret = avcodec_send_frame(ctx->audio_codec_ctx, frame);
    av_frame_unref(frame);
    if (ret < 0) {
        av_error_fmt(ctx->error_str,
                     "error sending frame to audio codec context!", ret);
        return ret;
    }
    return send_packets(ctx, ctx->audio_codec_ctx, ctx->audio_stream_index);
}

int
send_packets(const FFmpegOutputCtx *ctx, AVCodecContext *codec_context,
             const int stream_index)
{
    int ret = 0;
    AVPacket *pkt = av_packet_alloc();

    while (ret >= 0) {
        ret = avcodec_receive_packet(codec_context, pkt);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_packet_free(&pkt);
            return 0;
        }

        if (ret < 0) {
            av_error_fmt(ctx->error_str,
                         "error receiving packet from codec context!", ret);
        }
        else {
            pkt->stream_index = stream_index;
            pkt->pts = av_rescale_q(
                    pkt->pts, codec_context->time_base,
                    ctx->o_ctx->streams[stream_index]->time_base);
            pkt->dts = av_rescale_q(
                    pkt->dts, codec_context->time_base,
                    ctx->o_ctx->streams[stream_index]->time_base);

            ret = av_interleaved_write_frame(ctx->o_ctx, pkt);
            if (ret < 0) {
                av_error_fmt(ctx->error_str,
                             "error writing frame to output context!", ret);
            }
        }

        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);

    return ret;
}
