#!/bin/bash
set -e

PI_HOST=pi@10.195.118.204
PI_DIR=/home/pi/async-audio-relay

rsync -avz --exclude '.git/' --exclude 'objs/' --exclude 'bin/' \
    --exclude '*.exe' --exclude '*.o' --exclude '.vscode/' \
    ./ $PI_HOST:$PI_DIR/

ssh $PI_HOST "cd $PI_DIR && fuser -k 1490/tcp || true && sleep 1 && make clean && make server"