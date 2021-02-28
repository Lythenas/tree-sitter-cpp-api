#!/usr/bin/env bash
set -ev

source "$(dirname "${BASH_SOURCE[0]}")/_env.sh"

pushd build
build_type=$(cmake -L .. | grep CMAKE_BUILD_TYPE | sed 's/CMAKE_BUILD_TYPE:STRING=//g')
coverage=$(cmake -L .. | grep COVERAGE | sed 's/COVERAGE:BOOL=//g')
popd

if [[ "$build_type" != "Coverage" ]]; then
    ./scripts/setup_build.sh "-DCMAKE_BUILD_TYPE=Coverage"
fi

pushd build
make TreeSitterWrapper-tests-coverage $@
popd
