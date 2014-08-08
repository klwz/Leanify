#include "swf.h"

const unsigned char Swf::header_magic[] = { 'F', 'W', 'S' };
const unsigned char Swf::header_magic_deflate[] = { 'C', 'W', 'S' };
const unsigned char Swf::header_magic_lzma[] = { 'Z', 'W', 'S' };

size_t Swf::Leanify(size_t size_leanified /*= 0*/)
{
    if (!is_recompress)
    {
        Move(size_leanified);
        return size;
    }

    unsigned char *in_buffer = (unsigned char *)fp + 8;
    uint32_t in_len = *(uint32_t *)(fp + 4) - 8;

    // if SWF is compressed, decompress it first
    if (*fp == 'C')
    {
        // deflate
        if (is_verbose)
        {
            std::cout << "SWF is compressed with deflate." << std::endl;
        }
        size_t s = 0;
        in_buffer = (unsigned char *)tinfl_decompress_mem_to_heap(in_buffer, size - 8, &s, TINFL_FLAG_PARSE_ZLIB_HEADER);
        if (!in_buffer || s != in_len)
        {
            std::cout << "SWF file corrupted!" << std::endl;
            mz_free(in_buffer);
            Move(size_leanified);
            return size;
        }
    }
    else if (*fp == 'Z')
    {
        // LZMA
        if (is_verbose)
        {
            std::cout << "SWF is compressed with LZMA." << std::endl;
        }
        // | 4 bytes         | 4 bytes   | 4 bytes       | 5 bytes    | n bytes   | 6 bytes         |
        // | 'ZWS' + version | scriptLen | compressedLen | LZMA props | LZMA data | LZMA end marker |
        unsigned char *dst_buffer = new unsigned char[in_len];
        size_t s = in_len, len = size - 12 - LZMA_PROPS_SIZE;
        in_buffer += 4;
        if (LzmaUncompress(dst_buffer, &s, in_buffer + LZMA_PROPS_SIZE, &len, in_buffer, LZMA_PROPS_SIZE) ||
            s != in_len || len != size - 18 - LZMA_PROPS_SIZE)
        {
            std::cout << "SWF file corrupted!" << std::endl;
            delete[] dst_buffer;
            Move(size_leanified);
            return size;
        }
        in_buffer = dst_buffer;
    }
    else if (is_verbose)
    {
        std::cout << "SWF is not compressed." << std::endl;
    }

    // recompress
    size_t s = in_len, props = LZMA_PROPS_SIZE;
    unsigned char *dst = new unsigned char[in_len + LZMA_PROPS_SIZE];
    // have to set writeEndMark to true
    if (LzmaCompress(dst + LZMA_PROPS_SIZE, &s, in_buffer, in_len, dst, &props, iterations < 9 ? iterations : 9, 1 << 24, -1, -1, -1, 128, -1))
    {
        std::cout << "LZMA compression failed." << std::endl;
        s = SIZE_MAX;
    }

    // free decompressed data
    if (*fp == 'C')
    {
        mz_free(in_buffer);
    }
    else if (*fp == 'Z')
    {
        delete[] in_buffer;
    }

    // write header
    fp -= size_leanified;
    memcpy(fp, header_magic_lzma, sizeof(header_magic_lzma));

    // write SWF version, at least 13 to support LZMA
    if (fp[3 + size_leanified] < 13)
    {
        fp[3] = 13;
    }
    else
    {
        fp[3] = fp[3 + size_leanified];
    }

    // decompressed size (including header)
    *(uint32_t *)(fp + 4) = in_len + 8;

    s += LZMA_PROPS_SIZE;
    if (s + 12 < size)
    {
        size = s + 12;
        *(uint32_t *)(fp + 8) = s - LZMA_PROPS_SIZE;
        memcpy(fp + 12, dst, s);
    }
    else if (size_leanified)
    {
        memmove(fp + 8, fp + 8 + size_leanified, size - 8);
    }

    delete[] dst;

    return size;
}

void Swf::Move(size_t size_leanified)
{
    if (size_leanified)
    {
        fp -= size_leanified;
        memmove(fp, fp + size_leanified, size);
    }
}