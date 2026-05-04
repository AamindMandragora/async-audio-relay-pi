#!/bin/bash
set -e

make client
if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]] || [[ "$OS" == "Windows_NT" ]]; then
    ./bin/client_app.exe "$@"
else
    ./bin/client_app "$@"
fi