// Copyright 2022 Alim Zanibekov
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <stdio.h>

#include <Processing.NDI.Lib.h>

#include "ffmpeg_output.h"
#include "frame_converter.h"
#include "util.h"

#define NDI_RECV_TIMEOUT 2000

typedef struct AppOptions {
    char ndi_input_addr[255];
    char output[255];
    char output_format[30];
    char video_encoder[40];
    char audio_encoder[40];
    int video_bitrate;
    int audio_bitrate;
} AppOptions;

AppOptions
read_params(int argc, char **argv);

void
find_ndi_source(NDIlib_source_t *source);

int
main(int argc, char **argv)
{
    AppOptions opts = read_params(argc, argv);

    if (!avcodec_find_encoder_by_name(opts.video_encoder)) {
        printf("[ERROR] codec '%s' not found\n", opts.video_encoder);
        return 1;
    }
    if (!avcodec_find_encoder_by_name(opts.audio_encoder)) {
        printf("[ERROR] codec '%s' not found\n", opts.audio_encoder);
        return 1;
    }

    if (!NDIlib_initialize()) {
        printf("[ERROR] Unable to initialize NDI library");
        return 1;
    }

    NDIlib_source_t source = {};

    if (!strlen(opts.ndi_input_addr)) {
        find_ndi_source(&source);
    }
    else {
        source.p_url_address = opts.ndi_input_addr;
    }

    NDIlib_recv_create_v3_t recv_create_desc = {
        .source_to_connect_to = source,
        .p_ndi_recv_name = "ndi-streamer",
        .bandwidth = NDIlib_recv_bandwidth_lowest,
    };

    NDIlib_recv_instance_t recv = NDIlib_recv_create_v3(&recv_create_desc);

    if (!recv) {
        printf("[ERROR] Unable to create NDI receiver instance");
        return 1;
    }

    char ffmpeg_output_format[30];
    if (strcmp(opts.output_format, "rtmp") == 0) {
        snprintf(ffmpeg_output_format, sizeof ffmpeg_output_format, "flv");
    }
    else {
        snprintf(ffmpeg_output_format, sizeof ffmpeg_output_format, "%s",
                 opts.output_format);
    }

    FFmpegOutputCtx *fa_ctx = new_ffmpeg_output_ctx();
    FrameConverterCtx *fc_ctx = new_frame_converter_ctx();

    AVDictionary *output_options = NULL;
    av_dict_set(&output_options, "max_interleave_delta", "0", 0);

    if (strcmp(opts.output_format, "rtsp") == 0) {
        av_dict_set(&output_options, "rtsp_transport", "tcp", 0);
    }

    eh_init();
    while (eh_alive()) {
        if (fa_ctx->output != NULL) {
            ffmpeg_output_close(fa_ctx);
#ifdef _WIN32
            Sleep(2000);
#else
            sleep(2);
#endif
        }

        if (ffmpeg_output_init(fa_ctx, ffmpeg_output_format, opts.output) < 0) {
            printf("[ERROR] %s", fa_ctx->error_str);
            continue;
        }

        NDIlib_video_frame_v2_t v_frame;
        NDIlib_audio_frame_v2_t a_frame;

        int width = 0, height = 0;
        AVRational frame_rate = {};

        while (eh_alive()) {
            if (NDIlib_recv_capture_v2(recv, &v_frame, NULL, NULL,
                                       NDI_RECV_TIMEOUT)
                == NDIlib_frame_type_video) {
                width = v_frame.xres;
                height = v_frame.yres;
                frame_rate.num = v_frame.frame_rate_N;
                frame_rate.den = v_frame.frame_rate_D;
                NDIlib_recv_free_video_v2(recv, &v_frame);
                break;
            }
        }

        ffmpeg_output_close_codecs(fa_ctx);

        ffmpeg_output_setup_video(fa_ctx, opts.video_encoder, width, height,
                                  frame_rate, opts.video_bitrate);
        ffmpeg_output_setup_audio(fa_ctx, opts.audio_encoder,
                                  opts.audio_bitrate);

        if (ffmpeg_output_write_header(fa_ctx, &output_options) < 0) {
            printf("[ERROR] %s", fa_ctx->error_str);
            continue;
        }

        fc_reset(fc_ctx);

        while (eh_alive()) {
            NDIlib_frame_type_e res = NDIlib_recv_capture_v2(
                    recv, &v_frame, &a_frame, NULL, NDI_RECV_TIMEOUT);

            if (res == NDIlib_frame_type_video) {
                if (width != v_frame.xres || height != v_frame.yres) {
                    break;
                }

                AVFrame *frame = fc_ndi_video_frame_to_avframe(
                        fc_ctx, fa_ctx->video_codec_ctx, &v_frame);

                NDIlib_recv_free_video_v2(recv, &v_frame);

                if (ffmpeg_output_send_video_frame(fa_ctx, frame) < 0) {
                    printf("[ERROR] %s", fa_ctx->error_str);
                    break;
                }
            }
            else if (res == NDIlib_frame_type_audio) {
                AVFrame *frame = fc_ndi_audio_frame_to_avframe(
                        fc_ctx, fa_ctx->audio_codec_ctx, &a_frame);

                if (!frame) {
                    continue;
                }
                if (ffmpeg_output_send_audio_frame(fa_ctx, frame) < 0) {
                    printf("[ERROR] %s", fa_ctx->error_str);
                    break;
                }
                NDIlib_recv_free_audio_v2(recv, &a_frame);

                while ((frame = fc_ndi_audio_frame_to_avframe(
                                fc_ctx, fa_ctx->audio_codec_ctx, NULL))
                       != NULL) {
                    if (ffmpeg_output_send_audio_frame(fa_ctx, frame) < 0) {
                        printf("[ERROR] %s", fa_ctx->error_str);
                        break;
                    }
                }
            }
        }
    }

    av_dict_free(&output_options);

    free_ffmpeg_output_ctx(&fa_ctx);

    free_frame_converter_ctx(&fc_ctx);

    NDIlib_recv_destroy(recv);

    NDIlib_destroy();
    return 0;
}

void
find_ndi_source(NDIlib_source_t *source)
{
    NDIlib_find_instance_t p_find = NDIlib_find_create_v2(NULL);
    if (!p_find) {
        printf("unable to create NDIlib_find_instance_t\n");
        exit(0);
    }

    const NDIlib_source_t *p_sources = NULL;

    for (;;) {
        uint32_t no_sources = 0;
        printf("Looking for sources");

        for (size_t i = 1; !no_sources; ++i) {
            printf(i % 4 == 0 ? "\033[3D   \033[3D" : ".");
            fflush(stdout);
            NDIlib_find_wait_for_sources(p_find, 1000);
            p_sources = NDIlib_find_get_current_sources(p_find, &no_sources);
        }

        if (!p_sources) {
            printf("\n\nNo sources available\n");
            break;
        }

        printf("\n\nAvailable NDI sources:\n");

        for (uint32_t i = 0; i < no_sources; ++i) {
            printf("    %i. %s (%s)\n", i + 1, p_sources[i].p_ndi_name,
                   p_sources[i].p_url_address);
        }

        for (;;) {
            printf("\nselect source (r - retry, e - exit): ");

            char input[100];

            if (fgets(input, sizeof(input), stdin) != NULL) {
                const size_t len = strlen(input);
                if (len > 0 && input[len - 1] == '\n') {
                    input[len - 1] = '\0';
                }
            }

            if (input[0] == 'e') {
                exit(0);
            }
            else if (input[0] == 'r') {
                break;
            }

            char *end;
            long si = strtol(input, &end, 10);
            if (end == input) {
                printf("couldn't convert input \"%s\" to number\n", input);
            }
            else if (si > no_sources + 1 || si < 1) {
                printf("no source with index %li\n", si);
            }
            else {
                *source = p_sources[si - 1];
                return;
            }
        }
    }
}

const ProgramOption options[] = {
    { "n,ndi_input",
      "NDI Source address (optional, by default found ndi sources are "
      "suggested)",
      0 },
    { "f,output_format", "rtsp, rtmp (optional, by default 'rtsp')", 0 },
    { "o,output",
      "output url (optional, by default 'rtsp://127.0.0.1:8554/live.sdp')", 0 },
    { "v,video_codec", "ffmpeg video encoder (optional, by default 'libvpx')",
      0 },
    { "a,audio_codec", "ffmpeg audio encoder (optional, by default 'libopus')",
      0 },
    { "h,help", "show help", 1 },
    { "video_bitrate", "video bitrate (optional, by default '30000000')", 0 },
    { "audio_bitrate", "audio bitrate (optional, by default '320000')", 0 },
    { NULL, NULL, 0 },
};

AppOptions
read_params(int argc, char **argv)
{
    AppOptions res = {};
    const ProgramOption *opt = NULL;

    OptionParserCtx *op_ctx = op_init(options);
    int c;
    char *end;

    sprintf(res.audio_encoder, "libopus");
    sprintf(res.video_encoder, "libvpx");
    sprintf(res.output_format, "rtsp");
    sprintf(res.output, "rtsp://127.0.0.1:8554/live.sdp");
    res.video_bitrate = 30000000;
    res.audio_bitrate = 320000;

    for (; (c = op_parse(argc, argv, op_ctx, &opt)) != -1;) {
        switch (c) {
        case 'n':
            snprintf(res.ndi_input_addr, sizeof res.ndi_input_addr, "%s",
                     optarg);
            break;
        case 'f':
            if (strcmp(optarg, "rtsp") != 0 && strcmp(optarg, "rtmp") != 0) {
                printf("output \"%s\" is not supported\n", optarg);
                op_free(&op_ctx);
                exit(0);
            }
            snprintf(res.output_format, sizeof res.output_format, "%s", optarg);
            break;
        case 'o':
            snprintf(res.output, sizeof res.output, "%s", optarg);
            break;
        case 'v':
            snprintf(res.video_encoder, sizeof res.video_encoder, "%s", optarg);
            break;
        case 'a':
            snprintf(res.audio_encoder, sizeof res.audio_encoder, "%s", optarg);
            break;
        case 'h':
            op_print_help(argv[0], op_ctx);
            op_free(&op_ctx);
            exit(0);
        default:
            if (opt == NULL) {
                break;
            }
            if (strcmp(opt->name, "video_bitrate") == 0) {
                long si = strtol(optarg, &end, 10);
                if (end == optarg) {
                    printf("couldn't convert \"%s\" to number\n", optarg);
                    op_free(&op_ctx);
                    exit(0);
                }
                else {
                    res.video_bitrate = (int)si;
                }
            }
            else if (strcmp(opt->name, "audio_bitrate") == 0) {
                long si = strtol(optarg, &end, 10);
                if (end == optarg) {
                    printf("couldn't convert \"%s\" to number\n", optarg);
                    op_free(&op_ctx);
                    exit(0);
                }
                else {
                    res.audio_bitrate = (int)si;
                }
            }
            break;
        }
    }
    op_free(&op_ctx);

    return res;
}
