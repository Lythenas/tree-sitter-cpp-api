#!/usr/bin/env bash
set -ex

source "$(dirname "${BASH_SOURCE[0]}")/_env.sh"

pushd build
make TreeSitterWrapper-tests
./tests/TreeSitterWrapper-tests "$@"
popd
