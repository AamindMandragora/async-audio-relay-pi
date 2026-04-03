#!/bin/bash
set -e

make client
./bin/client_app.exe "$@"