FROM nvidia/cuda:12.4.0-devel-ubuntu22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt update && apt install -y \
    cmake \
    git \
    pkg-config \
    libcxxopts-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstrtspserver-1.0-dev \
    libbenchmark-dev \
    libgoogle-glog-dev \
    && rm -rf /var/lib/apt/lists/*

# Install NVML and dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    libnvidia-ml-dev \
    && rm -rf /var/lib/apt/lists/*

# OpenCV dependencies
RUN ln -sf /usr/bin/gfortran-12 /usr/bin/gfortran || true && \
    apt-get update &&\
    apt-get install -y \
    libblas-dev \
    liblapack-dev \
    libatlas-base-dev \
    gfortran \
    liblapack-dev \
    python3-numpy

# Build and Install OpenCV 4.8.0
RUN apt-get update &&\
    apt-get install -y --no-install-recommends \
    libatlas-base-dev \
    libgflags-dev \
    libgoogle-glog-dev \
    libhdf5-serial-dev \
    libleveldb-dev \
    liblmdb-dev \
    libprotobuf-dev \
    libsnappy-dev \
    protobuf-compiler \
    && \
    git clone --depth 10 --branch 4.8.0 https://github.com/opencv/opencv ~/opencv && \
    mkdir -p ~/opencv/build && cd ~/opencv/build && \
    cmake -D CMAKE_BUILD_TYPE=RELEASE \
    -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D WITH_IPP=OFF \
    -D WITH_CUDA=OFF \
    -D WITH_FFMPEG=ON \
    -D WITH_OPENCL=OFF \
    -D BUILD_TESTS=OFF \
    -D BUILD_PERF_TESTS=OFF \
    -D BUILD_DOCS=OFF \
    -D BUILD_EXAMPLES=OFF \
    .. && \
    make -j"$(nproc)" install && \
    ln -s /usr/local/include/opencv4/opencv2 /usr/local/include/opencv2

WORKDIR /workspace

CMD ["/bin/bash"]
