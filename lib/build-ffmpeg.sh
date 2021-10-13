#!/bin/bash

set -e

SCRIPT_DIR="$(dirname $0)"
FFMPEG_DIR="${SCRIPT_DIR}/../ffmpeg"
FF_DIR="${SCRIPT_DIR}/../ff"
INSTALL_DIR="$(pwd)/lib"

if [ -e "${FFMPEG_DIR}" ]; then
  cd "${FFMPEG_DIR}"
fi

if [ -e "${FF_DIR}" ]; then
  INSTALL_DIR="${FF_DIR}"
fi

CPPFLAGS="-D_POSIX_C_SOURCE=200112 -D_XOPEN_SOURCE=600" \
emconfigure ./configure \
    --prefix="${INSTALL_DIR}" \
    --cc="emcc" \
    --cxx="em++" \
    --ar="emar" \
    --ranlib="emranlib" \
    --target-os=none \
    --enable-cross-compile \
    --enable-lto \
    --cpu=generic \
    --arch=x86_64 \
    --disable-asm \
    --disable-inline-asm \
    --disable-programs \
    --disable-avdevice \
    --disable-doc \
    --disable-swresample \
    --disable-postproc  \
    --disable-avfilter \
    --disable-pthreads \
    --disable-w32threads \
    --disable-os2threads \
    --disable-network \
    --disable-logging \
    --disable-everything \
    --enable-gpl \
    --enable-version3 \
    --enable-static \
    --enable-demuxers \
    --enable-parsers \
    --enable-decoder=pcm_mulaw \
    --enable-decoder=pcm_alaw \
    --enable-decoder=adpcm_ima_smjpeg \
    --enable-protocol=file \
    --enable-protocol=pipe \
    --enable-decoder=h264 \
    --enable-decoder=hevc

make && make install
