FROM          alpine:3.7 as build

ENV           containerd_DIR /containerd-cpu
ENV           containerd_BUILD_DIR $containerd_DIR/build

RUN           apk --no-cache add build-base cmake curl git libuv-dev
# Use containerd v2.4.2
RUN           git clone https://github.com/OpenAINetwork/containerd.git $containerd_DIR && cd $containerd_DIR && \
    git reset --hard 0d1f36767253da24325706b049d2d8b931021175
RUN           mkdir $containerd_BUILD_DIR && cd $containerd_BUILD_DIR && \
    cmake .. -DWITH_HTTPD=OFF -DWITH_AEON=OFF && make
RUN           mv $containerd_BUILD_DIR/containerd /usr/bin/

FROM          alpine:3.7
RUN           apk --no-cache add libuv-dev
COPY          --from=build /usr/bin/containerd /usr/bin/
ENTRYPOINT    ["containerd"]