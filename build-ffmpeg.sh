set -e

CPPFLAGS="-D_POSIX_C_SOURCE=200112 -D_XOPEN_SOURCE=600" \
emconfigure ./configure \
    --prefix=$(pwd)/dist \
    --cc="emcc" \
    --cxx="em++" \
    --ar="emar" \
    --ranlib="emranlib" \
    --disable-asm \
    --disable-inline-asm \
    --cpu=generic \
    --target-os=none \
    --arch=x86_64 \
    --enable-gpl \
    --enable-version3 \
    --enable-cross-compile \
    --disable-logging \
    --disable-programs \
    --disable-ffmpeg \
    --enable-static \
    --enable-decoder=pcm_mulaw \
    --enable-decoder=pcm_alaw \
    --enable-decoder=adpcm_ima_smjpeg \
    --enable-lto \
    --disable-ffplay \
    --disable-ffprobe \
    --disable-doc \
    --disable-swresample \
    --disable-postproc  \
    --disable-avfilter \
    --disable-pthreads \
    --disable-w32threads \
    --disable-os2threads \
    --disable-network \
    --disable-everything \
    --enable-decoder=h264 \
    --enable-decoder=hevc \

make && make install

emcc ./dist/lib/libavcodec.a ./dist/lib/libavutil.a ./dist/lib/libswscale.a \
-s RESERVED_FUNCTION_POINTERS=1 \
-s INLINING_LIMIT=1 \
-s ALLOW_MEMORY_GROWTH=1 \
-s ABORTING_MALLOC=0 \
-s DISABLE_EXCEPTION_CATCHING=0 \
-s TOTAL_MEMORY=268435456 \
-s EXPORTED_FUNCTIONS="[ \
    '_avcodec_register_all', \
    '_avcodec_find_decoder', \
    '_avcodec_alloc_context3', \
    '_avcodec_free_context', \
    '_avcodec_open2', \
    '_av_free', \
    '_av_frame_alloc', \
    '_avcodec_close', \
    '_avcodec_send_packet', \
    '_av_init_packet', \
    '_av_packet_unref', \
    '_sws_freeContext', \
    '_sws_getContext', \
    '_sws_scale', \
    '_av_new_packet', \
    '_av_malloc', \
    '_avpicture_get_size', \
    '_avpicture_fill', \
    '_av_get_cpu_flags', \
    '_av_dict_set', \
    '_av_dict_free']" \
-s EXTRA_EXPORTED_RUNTIME_METHODS="['cwrap','ccall','addFunction']" \
--llvm-lto 1 \
--memory-init-file 0 -O3 \
-o ffmpeg.js
