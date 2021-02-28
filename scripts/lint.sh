#!/usr/bin/env bash
set -ev

CLANG_TIDY=${CLANG_TIDY:-clang-tidy}

DIR=$(dirname "${BASH_SOURCE[0]}")
source "$DIR/_common.sh"

"$DIR/run-clang-tidy.py" -clang-tidy-binary "$CLANG_TIDY" $FILES "$@"
