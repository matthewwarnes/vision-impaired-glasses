#!/bin/bash
# => 1. Enviroment/error handling setup
set -e
# Return to caller directory function
retCallerDir() {
  popd > /dev/null
}
# ('cd' to script directory while keeping on the stack the caller directory)
pushd $(dirname "$(readlink -f "$0")") > /dev/null
# Arm trap to return to caller directory on error condition
trap 'retCallerDir' ERR

BUILD_DIRECTORY_PATH="./build"
BIN_DIRECTORY_PATH="./bin"
# <====================================

# => 2. Actual functionality
# Build in Release (no test built)
cd ${BUILD_DIRECTORY_PATH}
cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j$(nproc) voice-control


# <=========================

# ('cd' back to caller directory)
retCallerDir
