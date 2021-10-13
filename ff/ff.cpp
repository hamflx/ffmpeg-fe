#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>

typedef int (*READ_PACKET)(void *opaque, uint8_t *buf, int buf_size);

#ifdef __cplusplus
extern "C"
{
#include <libavutil/log.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
#endif

extern "C" void log_callback(void *avcl, int level, const char *fmt, va_list vl)
{
  char log[1024] = {0};
  int n = vsnprintf(log, 1024, fmt, vl);

  if (n > 0 && log[n - 1] == '\n')
    log[n - 1] = 0;

  if (strlen(log) == 0)
    return;

  if (level <= AV_LOG_INFO)
  {
    printf("%s\n", log);
  }
}

enum class DECODER_ERROR
{
  SUCCESS = 0,
  NEED_MORE_DATA,
  AV_ERROR,
  NO_VIDEO_STREAM_FOUND,
  NO_DECODER_FOUND,
  OUT_OF_MEMORY,
  END_OF_FILE,
  META_INFO,
  AV_READ_FRAME,
  AVCODEC_SEND_PACKET,
  AVCODEC_RECEIVE_FRAME
};

struct VIDEO_META_INFO
{
  std::string sFormatName;
  std::string sCodecName;
  unsigned int nWidth;
  unsigned int nHeight;
};

struct DecoderInitializeException : public std::exception
{
  const char *what() const throw()
  {
    return "初始化失败！";
  }
};

class Decoder
{
private:
  VIDEO_META_INFO m_videoInfo;
  emscripten::val m_jsUpstream;
  bool m_bInitlized = false;
  AVFormatContext *m_pFmtCtx = NULL;
  AVCodecContext *m_pCodecCtx = NULL;
  AVFrame *m_pFrame = NULL;
  AVPacket *m_pPacketFrame = NULL;
  int m_iVideoStream = -1;

public:
  Decoder(emscripten::val jsUpstream);
  ~Decoder();

  DECODER_ERROR Initialize();
  emscripten::val Next();
  int ReadPacket(void *opaque, uint8_t *buf, int buf_size);
};

EMSCRIPTEN_BINDINGS(DecoderClass)
{
  emscripten::class_<Decoder>("Decoder")
      .constructor<emscripten::val>()
      .function("next", &Decoder::Next);
}

int DecoderReadPacket(void *opaque, uint8_t *buf, int buf_size)
{
  Decoder *pDecoder = (Decoder *)opaque;
  return pDecoder->ReadPacket(opaque, buf, buf_size);
}

Decoder::Decoder(emscripten::val jsUpstream) : m_jsUpstream(jsUpstream)
{
}

Decoder::~Decoder()
{
  if (m_pPacketFrame)
  {
    av_packet_free(&m_pPacketFrame);
  }
  if (m_pFrame)
  {
    av_frame_free(&m_pFrame);
  }
}

DECODER_ERROR Decoder::Initialize()
{
  if (m_bInitlized)
  {
    return DECODER_ERROR::SUCCESS;
  }

  av_log_set_callback(log_callback);

  int nBufferSize = 32768;
  unsigned char *pReadBuffer = (unsigned char *)av_malloc(nBufferSize);
  if (pReadBuffer == NULL)
  {
    return DECODER_ERROR::AV_ERROR;
  }

  AVIOContext *pIoCtx = avio_alloc_context(pReadBuffer, nBufferSize, 0, (void *)this, DecoderReadPacket, NULL, NULL);
  if (pIoCtx == NULL)
  {
    return DECODER_ERROR::AV_ERROR;
  }

  m_pFmtCtx = avformat_alloc_context();
  m_pFmtCtx->pb = pIoCtx;
  m_pFmtCtx->flags = AVFMT_FLAG_CUSTOM_IO;

  int ret;
  while ((ret = avformat_open_input(&m_pFmtCtx, NULL, NULL, NULL)) == AVERROR(EAGAIN))
  {
  }
  if (ret)
  {
    return DECODER_ERROR::AV_ERROR;
  }

  // 找到视频流。
  for (unsigned int i = 0; i < m_pFmtCtx->nb_streams; i++)
  {
    if (m_pFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      m_iVideoStream = i;
      break;
    }
  }
  if (m_iVideoStream == -1)
  {
    return DECODER_ERROR::NO_VIDEO_STREAM_FOUND;
  }

  // 根据视频的编码方式，创建解码器。
  AVCodecParameters *pCodecParams = m_pFmtCtx->streams[m_iVideoStream]->codecpar;
  const AVCodec *pCodec = avcodec_find_decoder(pCodecParams->codec_id);
  if (pCodec == NULL)
  {
    return DECODER_ERROR::NO_DECODER_FOUND;
  }

  // 保存视频信息。
  m_videoInfo.sFormatName = m_pFmtCtx->iformat->name;
  m_videoInfo.sCodecName = pCodec->name;

  // 构造解码器上下文。
  m_pCodecCtx = avcodec_alloc_context3(pCodec);
  if (m_pCodecCtx == NULL)
  {
    return DECODER_ERROR::AV_ERROR;
  }
  if (avcodec_parameters_to_context(m_pCodecCtx, pCodecParams) < 0)
  {
    return DECODER_ERROR::AV_ERROR;
  }
  if (avcodec_open2(m_pCodecCtx, pCodec, NULL) != 0)
  {
    return DECODER_ERROR::AV_ERROR;
  }

  m_videoInfo.nWidth = m_pCodecCtx->width;
  m_videoInfo.nHeight = m_pCodecCtx->height;

  m_pPacketFrame = av_packet_alloc();
  m_pFrame = av_frame_alloc();

  return DECODER_ERROR::SUCCESS;
}

emscripten::val Decoder::Next()
{
  emscripten::val result = emscripten::val::object();

  int ret = 0;

  if (!m_bInitlized)
  {
    DECODER_ERROR nError = Initialize();
    if (nError != DECODER_ERROR::SUCCESS)
    {
      throw DecoderInitializeException();
    }
    m_bInitlized = true;

    emscripten::val jsVideoInfo = emscripten::val::object();
    jsVideoInfo.set("format", m_videoInfo.sFormatName);
    jsVideoInfo.set("codec", m_videoInfo.sCodecName);
    jsVideoInfo.set("width", m_videoInfo.nWidth);
    jsVideoInfo.set("height", m_videoInfo.nHeight);
    result.set("status", (int)DECODER_ERROR::META_INFO);
    result.set("value", jsVideoInfo);
    return result;
  }

  while ((ret = avcodec_receive_frame(m_pCodecCtx, m_pFrame)) == AVERROR(EAGAIN))
  {
    while (1)
    {
      ret = av_read_frame(m_pFmtCtx, m_pPacketFrame);
      if (ret == 0)
      {
        if (m_pPacketFrame->stream_index == m_iVideoStream)
        {
          break;
        }
        av_packet_unref(m_pPacketFrame);
        continue;
      }

      printf("==> av_read_frame error: %s\n", av_err2str(ret));
      av_packet_unref(m_pPacketFrame);

      if (ret == AVERROR_EOF)
      {
        result.set("status", (int)DECODER_ERROR::END_OF_FILE);
        return result;
      }
      if (ret != 0)
      {
        printf("av_read_frame failed: %s\n", av_err2str(ret));
        result.set("status", (int)DECODER_ERROR::AV_READ_FRAME);
        return result;
      }
    }

    ret = avcodec_send_packet(m_pCodecCtx, m_pPacketFrame);
    av_packet_unref(m_pPacketFrame);

    if (ret != 0)
    {
      printf("==> avcodec_send_packet error: %s\n", av_err2str(ret));
      result.set("status", (int)DECODER_ERROR::AVCODEC_SEND_PACKET);
      return result;
    }
  }

  if (ret != 0)
  {
    printf("==> avcodec_receive_frame error: %s\n", av_err2str(ret));
    result.set("status", (int)DECODER_ERROR::AVCODEC_RECEIVE_FRAME);
    return result;
  }

  const auto nPixels = m_pFrame->width * m_pFrame->height;
  emscripten::val yFrame{emscripten::typed_memory_view(nPixels, m_pFrame->data[0])};
  emscripten::val uFrame{emscripten::typed_memory_view(nPixels / 4, m_pFrame->data[1])};
  emscripten::val vFrame{emscripten::typed_memory_view(nPixels / 4, m_pFrame->data[2])};

  emscripten::val Uint8Array = emscripten::val::global("Uint8Array");
  emscripten::val buffer = Uint8Array.new_(nPixels * 3 / 2);

  buffer.call<void>("set", yFrame);
  buffer.call<void>("set", uFrame, nPixels);
  buffer.call<void>("set", vFrame, nPixels + nPixels / 4);

  av_frame_unref(m_pFrame);

  result.set("status", (int)DECODER_ERROR::SUCCESS);
  result.set("value", buffer);
  return result;
}

int Decoder::ReadPacket(void *opaque, uint8_t *buf, int buf_size)
{
  emscripten::val packet = m_jsUpstream.call<emscripten::val>("next", buf_size).await();
  emscripten::val data = packet["data"];
  emscripten::val done = packet["done"];
  if (done.as<bool>())
  {
    return 0;
  }

  const auto nPacketLength = data["length"].as<unsigned>();
  if (nPacketLength > buf_size)
  {
    printf("==> nPacketLength > buf_size\n");
  }

  emscripten::val memoryView{emscripten::typed_memory_view(nPacketLength, buf)};
  memoryView.call<void>("set", data.call<emscripten::val>("slice", 0, nPacketLength));

  return nPacketLength;
}
