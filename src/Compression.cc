#include "Compression.h"

#include <stdexcept>
#include <vector>
#include <cstring>
#include <istream>
#include <ostream>

#include <zstd.h>
#include <lz4.h>
#include <lz4frame.h>

static std::vector<char> compressZstd(const char *src, size_t size) {
  size_t bound = ZSTD_compressBound(size);
  std::vector<char> out(bound);
  size_t ret = ZSTD_compress(out.data(), out.size(), src, size, 1);
  if (ZSTD_isError(ret)) {
    throw std::runtime_error("ZSTD compress error");
  }
  out.resize(ret);
  return out;
}

static std::vector<char> compressLz4(const char *src, size_t size) {
  int bound = LZ4_compressBound(static_cast<int>(size));
  std::vector<char> out(static_cast<size_t>(bound));
  int ret = LZ4_compress_default(src, out.data(), static_cast<int>(size), bound);
  if (ret <= 0) {
    throw std::runtime_error("LZ4 compress error");
  }
  out.resize(static_cast<size_t>(ret));
  return out;
}

static void decompressZstd(const char *src, size_t cSize, char *dst, size_t dSize) {
  size_t ret = ZSTD_decompress(dst, dSize, src, cSize);
  if (ZSTD_isError(ret) || ret != dSize) {
    throw std::runtime_error("ZSTD decompress error");
  }
}

static void decompressLz4(const char *src, size_t cSize, char *dst, size_t dSize) {
  int ret = LZ4_decompress_safe(src, dst, static_cast<int>(cSize), static_cast<int>(dSize));
  if (ret < 0 || static_cast<size_t>(ret) != dSize) {
    throw std::runtime_error("LZ4 decompress error");
  }
}

std::vector<char> compressBuffer(const char *src, size_t size, CompressionType type) {
  if (type == CompressionType::None) {
    return std::vector<char>(src, src + size);
  } else if (type == CompressionType::Zstd) {
    return compressZstd(src, size);
  } else if (type == CompressionType::Lz4) {
    return compressLz4(src, size);
  }
  throw std::runtime_error("Unsupported compression type");
}

void decompressBuffer(const char *src, size_t csize, char *dst, size_t dsize, CompressionType type) {
  if (type == CompressionType::None) {
    std::memcpy(dst, src, dsize);
    return;
  } else if (type == CompressionType::Zstd) {
    decompressZstd(src, csize, dst, dsize);
    return;
  } else if (type == CompressionType::Lz4) {
    decompressLz4(src, csize, dst, dsize);
    return;
  }
  throw std::runtime_error("Unsupported compression type");
}

void compressToStream(const char *src, size_t size, CompressionType type, std::ostream &out) {
  if (type == CompressionType::None) {
    out.write(src, size);
    return;
  } else if (type == CompressionType::Zstd) {
    const size_t chunk = 1 << 16;
    ZSTD_CCtx *cctx = ZSTD_createCCtx();
    if (!cctx) throw std::runtime_error("ZSTD createCCtx failed");
    size_t const cLevel = 1;
    size_t ret = ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, cLevel);
    if (ZSTD_isError(ret)) {
      ZSTD_freeCCtx(cctx);
      throw std::runtime_error("ZSTD setParameter failed");
    }
    std::vector<char> outBuf(ZSTD_CStreamOutSize());
    ZSTD_inBuffer input{src, size, 0};
    while (input.pos < input.size) {
      size_t toRead = std::min(chunk, input.size - input.pos);
      ZSTD_inBuffer chunkIn{src + input.pos, toRead, 0};
      while (chunkIn.pos < chunkIn.size) {
        ZSTD_outBuffer outBuffer{outBuf.data(), outBuf.size(), 0};
        size_t r = ZSTD_compressStream2(cctx, &outBuffer, &chunkIn, ZSTD_e_continue);
        if (ZSTD_isError(r)) {
          ZSTD_freeCCtx(cctx);
          throw std::runtime_error("ZSTD compressStream2 error");
        }
        out.write(outBuf.data(), outBuffer.pos);
      }
      input.pos += toRead;
    }
    ZSTD_outBuffer outBuffer{outBuf.data(), outBuf.size(), 0};
    size_t r = ZSTD_compressStream2(cctx, &outBuffer, &input, ZSTD_e_end);
    if (ZSTD_isError(r)) {
      ZSTD_freeCCtx(cctx);
      throw std::runtime_error("ZSTD finalize error");
    }
    out.write(outBuf.data(), outBuffer.pos);
    ZSTD_freeCCtx(cctx);
    return;
  } else if (type == CompressionType::Lz4) {
    LZ4F_compressionContext_t cctx;
    size_t const createRes = LZ4F_createCompressionContext(&cctx, LZ4F_VERSION);
    if (LZ4F_isError(createRes)) {
      throw std::runtime_error("LZ4F createCompressionContext failed");
    }
    LZ4F_preferences_t prefs{};
    std::vector<char> outBuf(1 << 16);
    size_t headerSize = LZ4F_compressBegin(cctx, outBuf.data(), outBuf.size(), &prefs);
    if (LZ4F_isError(headerSize)) {
      LZ4F_freeCompressionContext(cctx);
      throw std::runtime_error("LZ4F compressBegin failed");
    }
    out.write(outBuf.data(), headerSize);
    size_t pos = 0;
    const size_t chunk = 1 << 16;
    while (pos < size) {
      size_t toRead = std::min(chunk, size - pos);
      size_t bound = LZ4F_compressBound(toRead, &prefs);
      if (outBuf.size() < bound) outBuf.resize(bound);
      size_t ret = LZ4F_compressUpdate(cctx, outBuf.data(), outBuf.size(), src + pos, toRead, nullptr);
      if (LZ4F_isError(ret)) {
        LZ4F_freeCompressionContext(cctx);
        throw std::runtime_error("LZ4F compressUpdate failed");
      }
      out.write(outBuf.data(), ret);
      pos += toRead;
    }
    size_t endSize = LZ4F_compressEnd(cctx, outBuf.data(), outBuf.size(), nullptr);
    if (LZ4F_isError(endSize)) {
      LZ4F_freeCompressionContext(cctx);
      throw std::runtime_error("LZ4F compressEnd failed");
    }
    out.write(outBuf.data(), endSize);
    LZ4F_freeCompressionContext(cctx);
    return;
  }
  throw std::runtime_error("Unsupported compression type");
}

void decompressFromStreamToRange(std::istream &in, CompressionType type, size_t originalSize,
                                 size_t offset, char *dst, size_t size) {
  if (offset + size > originalSize) {
    throw std::runtime_error("Requested range exceeds original size");
  }
  if (type == CompressionType::None) {
    in.seekg(offset);
    in.read(dst, size);
    if (!in.good()) throw std::runtime_error("istream read error");
    return;
  } else if (type == CompressionType::Zstd) {
    ZSTD_DCtx *dctx = ZSTD_createDCtx();
    if (!dctx) throw std::runtime_error("ZSTD createDCtx failed");
    const size_t inChunk = ZSTD_DStreamInSize();
    std::vector<char> inBuf(inChunk);
    size_t produced = 0;
    size_t consumedOriginal = 0;
    while (produced < size) {
      in.read(inBuf.data(), inBuf.size());
      size_t got = static_cast<size_t>(in.gcount());
      ZSTD_inBuffer inB{inBuf.data(), got, 0};
      while (inB.pos < inB.size) {
        size_t remainOriginal = originalSize - consumedOriginal;
        size_t needPos = offset - consumedOriginal;
        size_t outAvail = size - produced;
        std::vector<char> tmp(std::min(remainOriginal, static_cast<size_t>(ZSTD_DStreamOutSize())));
        ZSTD_outBuffer outB{tmp.data(), tmp.size(), 0};
        size_t r = ZSTD_decompressStream(dctx, &outB, &inB);
        if (ZSTD_isError(r)) {
          ZSTD_freeDCtx(dctx);
          throw std::runtime_error("ZSTD decompressStream error");
        }
        if (needPos < outB.pos) {
          size_t start = needPos;
          size_t cp = std::min(outB.pos - start, outAvail);
          std::memcpy(dst + produced, tmp.data() + start, cp);
          produced += cp;
          offset += cp;
        }
        consumedOriginal += outB.pos;
        if (produced >= size) break;
      }
      if (got == 0) break;
    }
    ZSTD_freeDCtx(dctx);
    if (produced != size) throw std::runtime_error("ZSTD ranged decode incomplete");
    return;
  } else if (type == CompressionType::Lz4) {
    LZ4F_decompressionContext_t dctx;
    size_t const createRes = LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
    if (LZ4F_isError(createRes)) {
      throw std::runtime_error("LZ4F createDecompressionContext failed");
    }
    const size_t inChunk = 1 << 16;
    std::vector<char> inBuf(inChunk);
    std::vector<char> outBuf(inChunk * 4);
    size_t produced = 0;
    size_t consumedOriginal = 0;
    while (produced < size) {
      in.read(inBuf.data(), inBuf.size());
      size_t srcSize = static_cast<size_t>(in.gcount());
      size_t srcPos = 0;
      while (srcPos < srcSize && produced < size) {
        size_t dstSize = outBuf.size();
        size_t srcChunk = srcSize - srcPos;
        size_t res = LZ4F_decompress(dctx, outBuf.data(), &dstSize, inBuf.data() + srcPos, &srcChunk, nullptr);
        if (LZ4F_isError(res)) {
          LZ4F_freeDecompressionContext(dctx);
          throw std::runtime_error("LZ4F decompress error");
        }
        srcPos += srcChunk;
        size_t needPos = offset - consumedOriginal;
        size_t outAvail = size - produced;
        if (needPos < dstSize) {
          size_t start = needPos;
          size_t cp = std::min(dstSize - start, outAvail);
          std::memcpy(dst + produced, outBuf.data() + start, cp);
          produced += cp;
          offset += cp;
        }
        consumedOriginal += dstSize;
        if (res == 0 && srcPos >= srcSize) break;
      }
      if (srcSize == 0) break;
    }
    LZ4F_freeDecompressionContext(dctx);
    if (produced != size) throw std::runtime_error("LZ4 ranged decode incomplete");
    return;
  }
  throw std::runtime_error("Unsupported compression type");
}
