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
cd ${BIN_DIRECTORY_PATH}
rm -f *
cd - > /dev/null

cd ${BUILD_DIRECTORY_PATH}
make clean > /dev/null 2>&1 ||  true
rm -rf *
# <=========================

# ('cd' back to caller directory)
retCallerDir
