# Running Locally

1. Clone the project
2. Build from source
```bash
mkdir build
cd build
cmake ..
make -j4
```
3. Run 24 rtsp servers, one per stream:
```bash
../scripts/run_rtsp_servers.sh <path_to_video.mp4>
```
4. In a new terminal, run the benchmarks:
```bash
./benchmarks
```

# Known Issues
Currently, the benchmark for 24 concurrent streams fails after opening 20 streams with error:
```bash
(benchmarks:1620079): GLib-ERROR **: 16:08:05.229: Creating pipes for GWakeup: Too many open files
[1]    1620079 trace trap (core dumped)  ./benchmarks
```
