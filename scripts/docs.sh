#!/usr/bin/env bash
set -ev

source "$(dirname "${BASH_SOURCE[0]}")/_env.sh"

pushd build
../extern/m.css/documentation/doxygen.py ../mcss-conf.py
popd
