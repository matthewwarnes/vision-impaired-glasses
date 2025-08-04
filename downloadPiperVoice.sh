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

voice="cori"
pitch="medium"
lang="en_GB"

src="https://huggingface.co/rhasspy/piper-voices"
pfx="resolve/main/ggml"

#https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_GB/cori/medium/en_GB-cori-medium.onnx?download=true

url="$src/resolve/main/en/$lang/$voice/$pitch/$lang-$voice-$pitch.onnx"

BOLD="\033[1m"
RESET='\033[0m'

# get the path of this script
get_script_path() {
    if [ -x "$(command -v realpath)" ]; then
        dirname "$(realpath "$0")"
    else
        _ret="$(cd -- "$(dirname "$0")" >/dev/null 2>&1 || exit ; pwd -P)"
        echo "$_ret"
    fi
}

script_path="$(get_script_path)"

# Check if the script is inside a /bin/ directory
case "$script_path" in
    */bin) default_download_path="$PWD" ;;  # Use current directory as default download path if in /bin/
    *) default_download_path="$script_path" ;;  # Otherwise, use script directory
esac

models_path="${2:-$default_download_path}"



# download ggml model

printf "Downloading piper model %s from '%s' ...\n" "$lang-$voice-$pitch.onnx" "$src"

cd models || exit

if [ -f "$lang-$voice-$pitch.onnx" ]; then
    printf "Model %s already exists. Skipping download.\n" "$lang-$voice-$pitch.onnx"
    exit 0
fi

if [ -x "$(command -v wget2)" ]; then
    wget2 --no-config --progress bar -O "$lang"-"$voice"-"$pitch".onnx $url
    wget2 --no-config --progress bar -O "$lang"-"$voice"-"$pitch".onnx.json "$url.json"
elif [ -x "$(command -v wget)" ]; then
    wget --no-config --quiet --show-progress -O "$lang"-"$voice"-"$pitch".onnx $url
    wget --no-config --quiet --show-progress -O "$lang"-"$voice"-"$pitch".onnx.json "$url.json"
elif [ -x "$(command -v curl)" ]; then
    curl -L --output "$lang"-"$voice"-"$pitch".onnx $url
    curl -L --output "$lang"-"$voice"-"$pitch".onnx.json "$url.json"
else
    printf "Either wget or curl is required to download models.\n"
    exit 1
fi

if [ $? -ne 0 ]; then
    printf "Failed to download model %s \n" "$lang-$voice-$pitch.onnx"
    printf "Please try again later or download the original Whisper model files and convert them yourself.\n"
    exit 1
fi

# ('cd' back to caller directory)
retCallerDir
