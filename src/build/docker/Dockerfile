# Ubuntu 22.04 provides a glibc 2.35 baseline. Run the ARM64 container natively
# on Apple Silicon and use its x86_64 cross compiler for the Linux game.
FROM ubuntu:22.04@sha256:2edbbc5dc405e9612ba3584ce95480277e3eb374407b5505fe26f17df77c7dbc
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    ca-certificates python3 python3-pip pkg-config g++-x86-64-linux-gnu \
    && python3 -m pip install --no-cache-dir SCons==4.8.1 \
    && rm -rf /var/lib/apt/lists/*
# SCons also invokes binutils by their unprefixed names.
RUN for tool in gcc g++ ar ranlib as strip objcopy objdump; do \
    ln -s /usr/bin/x86_64-linux-gnu-$tool /usr/local/bin/$tool; \
    done
