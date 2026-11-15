# Ada Docker Container for test compilation :)

FROM debian:trixie-slim

ENV DEBIAN_FRONTEND=noninteractive \
    SDL_VIDEODRIVER=wayland \
    XDG_RUNTIME_DIR=/tmp

WORKDIR /Ada

RUN echo "=== Installing dependencies... ===" && \
    sed -i 's/Components: main/Components: main contrib non-free non-free-firmware/g' /etc/apt/sources.list.d/debian.sources && \
    apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    build-essential \
    git \
    cmake \
    pkg-config \
    libsdl2-dev \
    libsdl2-image-dev \
    libsdl2-ttf-dev \
    libsdl2-mixer-dev \
    libttspico-dev \
    libttspico-utils \
    libnotify-dev \
    libcurl4-openssl-dev \
    libwayland-client0 \
    libwayland-egl1 \
    libxkbcommon0 \
    libgl1-mesa-dri \
    libegl1 \
    libpulse0 \
    pipewire-audio-client-libraries \
    xterm \
    && rm -rf /var/lib/apt/lists/*

RUN ln -sf /usr/bin/xterm /usr/local/bin/x-terminal-emulator

WORKDIR /Ada/Ada-Virtual-Assistant

RUN echo "=== Well... It's time to copy Ada source code :D ==="
COPY . .

RUN echo "=== LET'S GO, COMPILING ADA... ===" && \
    make -j$(nproc)

WORKDIR /Ada/Ada-Virtual-Assistant/Ada_packed

CMD ["sh", "-c", "echo 'Ada Docker Container 1.0 by Juan Yaguaro (aka silverhacker)' && ./Ada --setup-only && sleep infinity"]