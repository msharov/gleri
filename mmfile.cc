#include "mmfile.h"
#include "gleri/gldefs.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <zlib.h>

void CMMFile::Open (const char* filename)
{
    _fd = open (filename, O_RDONLY);
    if (_fd < 0)
	throw XError ("%s %s: %s", "open", filename, strerror(errno));
    _sz = lseek (_fd, 0, SEEK_END);
    _p = (GLubyte*) mmap (nullptr, _sz, PROT_READ, MAP_PRIVATE, _fd, 0);
    if (_p == MAP_FAILED) {
	close (_fd);
	throw XError ("%s %s: %s", "mmap", filename, strerror(errno));
    }
}

void CMMFile::Close (void) noexcept
{
    munmap (_p, _sz);
    close (_fd);
}

/*static*/ CMMFile::pointer CMMFile::DecompressBlock (const_pointer p, size_type isz, size_type& osz)
{
    z_stream zs;
    memset (&zs, 0, sizeof(zs));

    zs.avail_in = isz;
    zs.next_in = const_cast<Bytef*>(p);
    GLuint bufsz = 4096, bread = 0;
    GLuint chunksz = bufsz;
    GLubyte* out = (GLubyte*) malloc (bufsz);
    zs.avail_out = bufsz;
    zs.next_out = out;

    enum { USE_GZIP_FORMAT = 16 };	// zlib fairy dust
    if (Z_OK != inflateInit2 (&zs, USE_GZIP_FORMAT+MAX_WBITS))
	throw XError ("gzip decompression error");

    for (;;) {
	int r = inflate (&zs, Z_NO_FLUSH);
	if (r == Z_STREAM_END)
	    break;
	else if (r == Z_OK) {
	    bread += chunksz-zs.avail_out;
	    out = (GLubyte*) realloc (out, bufsz*2);
	    bufsz *= 2;
	    zs.next_out = out+bread;
	    zs.avail_out = chunksz = bufsz-bread;
	} else {
	    inflateEnd (&zs);
	    throw XError ("gzip decompression error");
	}
    }
    osz = bread + chunksz-zs.avail_out;
    inflateEnd (&zs);
    return (out);
}
