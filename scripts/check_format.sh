#!/usr/bin/env bash
set -evx

CLANG_FORMAT=${CLANG_FORMAT:-clang-format}

DIR=$(dirname "${BASH_SOURCE[0]}")
source "$DIR/_common.sh"

exec "$DIR/run-clang-format.py" \
    --clang-format-executable "$CLANG_FORMAT" \
    --style file \
    $FILES
