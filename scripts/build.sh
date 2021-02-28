#!/usr/bin/env bash
set -ev

source "$(dirname "${BASH_SOURCE[0]}")/_env.sh"

pushd build
make $@
popd
