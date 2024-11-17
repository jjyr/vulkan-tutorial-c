#!/bin/bash

CC=cc
FLAGS="-I /opt/homebrew/include -L/opt/homebrew/lib/ -lglfw -lvulkan.1"
DYLD_LIBRARY_PATH="/opt/homebrew/opt/vulkan-validationlayers/lib/"

build() {
  local filename=$1
  local source_file="src/${filename}.c"
  local output_file="build/${filename}"

  # check source
  if [ ! -f "${source_file}" ]; then
    echo "Error: file $source_file not exist"
    return 1
  fi

  # compile source
  echo "Compiling $source_file ..."
  $CC $FLAGS "$source_file" -o "$output_file"

  # check result
  if [ $? -eq 0 ]; then
    echo "Success, output $output_file"
  else
    echo "Failed"
    return 1
  fi
}

if [ $# -eq 0 ]; then
  echo "Build all..."
  build "triangle"
else
  build $1

  if [ "${2}" = "--run" ]; then
    OUT="build/${1}"
    echo "Run $OUT"
    bash -c "DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH $OUT"
  fi
fi
