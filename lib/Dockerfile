FROM emscripten/emsdk

RUN mkdir /ffmpeg
WORKDIR /ffmpeg

RUN git init
RUN git remote add origin https://git.ffmpeg.org/ffmpeg.git
RUN git fetch --depth 1 origin n4.3.2
RUN git checkout FETCH_HEAD

COPY build-ffmpeg.sh /ffmpeg/
RUN sh /ffmpeg/build-ffmpeg.sh

CMD bash
