#!/usr/bin/env bash
set -ev

mkdir -pv build
pushd build

rm -f CMakeCache.txt
rm -rf CMakeFiles

cmake .. $@
popd
