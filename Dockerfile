FROM debian:bookworm AS build

RUN apt update && apt install -y build-essential cmake wget \
    pkg-config avahi-daemon libavahi-client3 libavformat-dev \
    libavcodec-dev libswresample-dev libswscale-dev libavutil-dev

WORKDIR /builder

RUN wget https://downloads.ndi.tv/SDK/NDI_SDK_Linux/Install_NDI_SDK_v6_Linux.tar.gz
RUN tar -zxf Install_NDI_SDK_v6_Linux.tar.gz && chmod +x Install_NDI_SDK_v6_Linux.sh
RUN echo Y | ./Install_NDI_SDK_v6_Linux.sh

COPY src src
COPY cmake cmake
COPY CMakeLists.txt .

RUN mkdir build && cd build && cmake \
    -DNDI_SDK_DIR="/builder/NDI SDK for Linux" \
    -DCMAKE_INSTALL_PREFIX=/usr/local/ \
    -DCMAKE_BUILD_TYPE=Release .. \
    && make install

FROM debian:bookworm

RUN apt update && apt install -y avahi-daemon libavahi-client3 ffmpeg

COPY --from=build /usr/local/bin/ndi-streamer /usr/local/bin/
COPY --from=build /usr/local/lib/libndi.* /usr/local/lib/

ENTRYPOINT ["ndi-streamer"]