#!/usr/bin/env bash

###########################################
### Run N rtsp servers (separate processes)
##########################################

if [ ! -f "$1" ]; then
    if [ -z "$1" ]; then
        echo "Usage: $0 <path_to_mp4>"
    else
        echo "$1 is not a file"
    fi
    exit 1
fi

read -p "Enter number of servers: " n

# Kill all background jobs on Interrupt (Ctrl+C)
trap 'echo "Stopping servers..."; kill 0; exit' INT

# Path to directory where this script is placed
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
for i in $(seq 1 "$n"); do
    "$SCRIPT_DIR/../build/rtsp_server" -m "/stream" -p $((8553 + i)) "$1" &
done

wait
