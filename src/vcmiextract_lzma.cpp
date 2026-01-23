#include "vcmiextract.h"

#include <cstring>
#include <lzma.h>

void vcmiextract::decompress_file_lzma(memory_file & source, memory_file & target)
{
    lzma_options_lzma opt;
    memset(&opt, 0, sizeof(opt));
    opt.dict_size = 256 * 1024;
    opt.lc        = 3;
    opt.lp        = 0;
    opt.pb        = 2;
    opt.mode      = LZMA_MODE_NORMAL;
    opt.nice_len  = 64;
    opt.mf        = LZMA_MF_HC3;
    opt.depth     = 0;

    lzma_filter filters[2];
    filters[0].id      = LZMA_FILTER_LZMA1;
    filters[0].options = &opt;
    filters[1].id      = LZMA_VLI_UNKNOWN;
    filters[1].options = nullptr;

    lzma_stream strm = LZMA_STREAM_INIT;
    strm.avail_out = target.size();
    strm.next_out = target.ptr();
    strm.avail_in = source.size() - 1;
    strm.next_in = source.ptr() + 1;

    // Initialize decoder for raw LZMA (no container like .xz)
    {
        lzma_ret ret = lzma_raw_decoder(&strm, filters);
        assert(ret == LZMA_OK);
    }

    {
        lzma_ret ret = lzma_code(&strm, LZMA_FINISH);
        assert(ret == LZMA_STREAM_END);
    }
    lzma_end(&strm);
}
