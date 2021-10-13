import { WebglScreen } from './webglScreen.js'

async function beginReadPacket() {
  let cachedSize = 0
  /**
   * @type {Uint8Array[]}
   */
  const cachedChunks = []
  const reader = (await fetch('test.mkv')).body.getReader()
  const combineChunks = size => {
    let resultSize = 0
    const result = new Uint8Array(size)
    while (resultSize < size) {
      const chunk = cachedChunks.shift()
      if (chunk.length + resultSize > size) {
        const needSize = size - resultSize
        result.set(chunk.slice(0, needSize), resultSize)
        resultSize += needSize
        cachedChunks.unshift(chunk.slice(needSize))
        break
      } else {
        result.set(chunk, resultSize)
        resultSize += chunk.length
      }
    }
    cachedSize -= result.length
    return result
  }
  return async size => {
    while (cachedSize < size) {
      const { done, value } = await reader.read()
      if (done) {
        if (!cachedSize) return { done }
        return { data: combineChunks(cachedSize) }
      }
      cachedChunks.push(value)
      cachedSize += value.length
    }
    return { data: combineChunks(size) }
  }
}

async function initialize() {
  const source = {
    next: await beginReadPacket()
  }
  const decoder = new Module.Decoder(source)
  const render = new WebglScreen(document.getElementById('canvas'))

  const { value: { width, height, codec, format } } = await decoder.next()
  const pixelCount = width * height
  render.setSize(width, height, width)
  console.log('==> video info', format, codec, width, height)

  const playFrame = async () => {

    const { status, value: frame } = await decoder.next()
    if (status === 0) {
      requestAnimationFrame(playFrame)

      const y = frame.slice(0, pixelCount)
      const u = frame.slice(pixelCount, pixelCount + pixelCount / 4)
      const v = frame.slice(pixelCount + pixelCount / 4)

      render.renderImg(width, height, y, u, v)
    } else {
      console.log('==> exit with status code: ' + status)
    }
  }

  playFrame()
}

window.initialize = initialize
