#!/usr/bin/env bash
set -ev

pushd build
bash <(curl -s https://codecov.io/bash) -f coverage.info.cleaned
popd
