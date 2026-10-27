#!/usr/bin/env bash
# Run two fio jobs at the same time (heavy + light) to give ptop
# something to measure. Watch them in ptop from another terminal.
# To run: tools/fio_dual.sh [runtime_s]   (default 30)

set -euo pipefail

RUNTIME=${1:-30}
DIR=/var/tmp/fio_dual                   # fio's test files and job files
OUT=$(cd "$(dirname "$0")" && pwd)      # results go next to this script
FIOOUTPUT1=$OUT/fio_dual1.out
FIOOUTPUT2=$OUT/fio_dual2.out

mkdir -p "$DIR"
trap 'rm -rf "$DIR"' EXIT               # delete test files when done or on Ctrl+C

# heavy: 16 workers, small writes, deep queue (~1.6-1.9 cores on this laptop)
cat > "$DIR/heavy.fio" <<EOF
[heavy]
ioengine=io_uring
direct=1
thread
time_based
runtime=$RUNTIME
directory=$DIR
filename=fiotest1
size=1G
rw=write
bs=4k
iodepth=64
numjobs=16
EOF

# light: 1 worker, big reads, shallow queue
cat > "$DIR/light.fio" <<EOF
[light]
ioengine=io_uring
direct=1
thread
time_based
runtime=$RUNTIME
directory=$DIR
filename=fiotest2
size=1G
rw=read
bs=1M
iodepth=4
numjobs=1
EOF

# start both at the same time, in the background
fio "$DIR/heavy.fio" --output="$FIOOUTPUT1" &
HEAVY_PID=$!
fio "$DIR/light.fio" --output="$FIOOUTPUT2" &
LIGHT_PID=$!

echo "running for ${RUNTIME}s: heavy pid=$HEAVY_PID, light pid=$LIGHT_PID (look for these in ptop)"

wait "$HEAVY_PID" "$LIGHT_PID"
echo "done: results in $FIOOUTPUT1 and $FIOOUTPUT2"
