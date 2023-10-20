# NDI streamer

`ndi-streamer` is a tool for re-streaming NDI stream as rtsp/rtmp

#### Usage

```sh
./ndi-streamer -n {ndi_addr} -o rtsp://127.0.0.1:8554/live.sdp
```

To display all available ndi sources, run the program without -n/--ndi_input option

```sh
./ndi-streamer
```

Options:

```shell
./ndi-streamer -h
```

```text
-n, --ndi_input: NDI Source address (optional, by default found ndi sources are suggested)
-f, --output_format: rtsp, rtmp (optional, by default 'rtsp')
-o, --output: output url (optional, by default 'rtsp://127.0.0.1:8554/live.sdp')
-v, --video_codec: ffmpeg video encoder (optional, by default 'libvpx')
-a, --audio_codec: ffmpeg audio encoder (optional, by default 'libopus')
-h, --help: show help
--video_bitrate: video bitrate (optional, by default '30000000')
--audio_bitrate: audio bitrate (optional, by default '320000')
```

#### Build

1. Set NDI and ffmpeg SDK paths in config.mak
2. `make clean build/ndi-streamer`
