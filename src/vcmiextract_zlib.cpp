#include "vcmiextract.h"

#include <zlib.h>

void vcmiextract::decompress_file(memory_file & source, memory_file & target)
{
	z_stream_s * inflate_state;

	inflate_state = new z_stream();
	inflate_state->zalloc = Z_NULL;
	inflate_state->zfree = Z_NULL;
	inflate_state->opaque = Z_NULL;
	inflate_state->avail_in = 0;
	inflate_state->next_in = Z_NULL;

	{
		int ret = inflateInit2(inflate_state, 15);
		assert(ret == Z_OK);
	}

	inflate_state->avail_out = target.size();
	inflate_state->next_out = target.ptr();
	inflate_state->avail_in = source.size();
	inflate_state->next_in = source.ptr();

	{
		int ret = inflate(inflate_state, Z_NO_FLUSH);
		assert(ret == Z_STREAM_END);
	}

	inflateEnd(inflate_state);

	delete inflate_state;
}
