FROM debian:stable-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ cmake make pkg-config \
    libasio-dev libssl-dev nlohmann-json3-dev rabbitmq-c-dev curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN rm -rf build && mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

EXPOSE 8081

CMD ["./build/server"]
