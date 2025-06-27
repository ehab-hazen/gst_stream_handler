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
