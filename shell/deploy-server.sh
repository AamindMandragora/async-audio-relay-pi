#!/bin/bash
set -e

PI_HOST=pi@10.195.118.204
PI_DIR=~/async-audio-relay

rsync -avz ./ $PI_HOST:$PI_DIR/
ssh $PI_HOST "cd $PI_DIR && git pull && fuser -k 1490/tcp; make server"