#!/bin/bash
set -e
docker run --rm -v /mnt/c/Users/adam/code/ataxx/mac:/src -w /src ghcr.io/autc04/retro68 "$@"

