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

# <====================================

# => 2. Actual functionality
# Build in Release (no test built)

./build/whispercpp/src/whispercpp/models/download-ggml-model.sh base.en
./build/whispercpp/src/whispercpp/models/download-vad-model.sh silero-v5.1.2
mv build/whispercpp/src/whispercpp/models/ggml*.bin ./

# <=========================

# ('cd' back to caller directory)
retCallerDir
