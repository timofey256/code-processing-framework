FROM debian:stable-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl-dev \
    g++ cmake make pkg-config \
    libasio-dev nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN rm -rf build && mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

EXPOSE 8081

CMD ["./build/server"]
