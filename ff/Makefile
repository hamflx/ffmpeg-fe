all: ../web/ff.js

rebuild: clean all

CC = emcc
CFLAGS = -O3 -I../lib/include
LIBOBJS := ../lib/lib/libavcodec.a ../lib/lib/libavutil.a ../lib/lib/libavformat.a
EMCCFLAGS = -gsource-map -g --bind -s ASYNCIFY -s WASM=1 -s ALLOW_TABLE_GROWTH=1 -s ALLOW_MEMORY_GROWTH=1 -s FILESYSTEM=0 -s ASSERTIONS=1

../web/ff.js: ff.cpp $(LIBOBJS)
	$(CC) $(EMCCFLAGS) $(CFLAGS) -o $@ $^

fflib: build-ffmpeg.sh
	sh build-ffmpeg.sh

clean:
	rm -f ff.js ff.wasm
