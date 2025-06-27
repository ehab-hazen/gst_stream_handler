FROM gcc:latest

# Install dependencies
RUN apt update && apt install -y \
    cmake \
    pkg-config \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstrtspserver-1.0-dev \
    libopencv-dev \
    libbenchmark-dev \
 && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy project files
COPY . .

# Create build directory and build
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make benchmarks

# Set working directory to build for execution
WORKDIR /app/build

# Run benchmarks by default
CMD ["./benchmarks"]
