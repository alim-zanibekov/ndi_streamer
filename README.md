# NDI Streamer

`ndi-streamer` is a lightweight tool for re-streaming NDI (Network Device Interface) streams as RTSP or RTMP.

---

### Usage

To re-stream an NDI source, use the following command:

```sh
./ndi-streamer -n {ndi_addr} -o rtsp://127.0.0.1:8554/live.sdp
# Examples
./ndi-streamer -n 127.0.0.1:5961 -f rtmp -v libx264 -a aac -o rtmp://10.10.0.100/live/test # rtmp stream
./ndi-streamer -n 127.0.0.1:5961 -f rtsp -v libx264 -a aac -o rtsp://10.10.0.100:8554/live.sdp # h264/aac rtsp stream
./ndi-streamer -n 127.0.0.1:5961 -f rtsp -o rtsp://10.10.0.100:8554/live.sdp # vp9/opus rtsp stream
```

### List Available NDI Sources

If you don't specify an NDI source, the program will list all available NDI sources:

```sh
./ndi-streamer
```

### Options

Run the following command to see a full list of options:

```sh
./ndi-streamer -h
```

| Option                  | Description                                                                           | Default Value                    |
|-------------------------|---------------------------------------------------------------------------------------|----------------------------------|
| `-n`, `--ndi_input`     | NDI source address (optional). <br/>If not provided, found NDI sources are suggested. |                                  |
| `-f`, `--output_format` | Output format: `rtsp` or `rtmp` (optional).                                           | `rtsp`                           |
| `-o`, `--output`        | Output URL (optional).                                                                | `rtsp://127.0.0.1:8554/live.sdp` |
| `-v`, `--video_codec`   | FFmpeg video encoder (optional).                                                      | `libvpx`                         |
| `-a`, `--audio_codec`   | FFmpeg audio encoder (optional).                                                      | `libopus`                        |
| `--video_bitrate`       | Video bitrate in bits per second (optional).                                          | `30000000`                       |
| `--audio_bitrate`       | Audio bitrate in bits per second (optional).                                          | `320000`                         |
| `-h`, `--help`          | Show help and exit.                                                                   |                                  |

---

### Build

To build `ndi-streamer`, ensure your system meets the following requirements:

- **NDI SDK**: Installed and properly configured.
- **FFmpeg libs**: `avformat`, `avcodec`, `swresample`, `swscale` and `avutil` installed and available on your system.
- **C toolchain**: A functional C/C++ compiler.

#### Build Steps

1. Create a build directory and navigate to it:
   ```sh
   mkdir build && cd build
   ```
2. Run CMake to configure the build:

   - On **Linux**, specify the path to the NDI SDK using the `-DNDI_SDK_DIR` flag:
     ```sh
     cmake -DNDI_SDK_DIR="{NDI SDK for Linux path}" ..
     ```
   - On **Windows**, specify the path to the [FFmpeg (shared)](https://github.com/GyanD/codexffmpeg/releases/tag/7.1) using the `-DFFMPEG_ROOT` flag:
     ```sh
     cmake -DCMAKE_INSTALL_PREFIX=C:\Users\User\Desktop -DFFMPEG_ROOT="{FFmpeg path}" ..
     ```
   - On **OSX**, all dependencies can be found automatically:
     ```sh
     cmake ..
     ```
3. Compile the project:
   ```sh
   cmake --build .
   ```
4. (Optional) Install the compiled binary:
   ```sh
   sudo cmake --build . --target install
   ```

