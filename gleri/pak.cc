#include "pak.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <zlib.h>

//{{{ File operations --------------------------------------------------

pakbuf_t ReadFileToPakbuf (const char* filename)
{
    pakbuf_t v;
    int fd = open (filename, O_RDONLY);
    if (fd >= 0) {
	struct stat st;
	if (0 == fstat (fd, &st) && S_ISREG(st.st_mode) && st.st_size < (1u<<23)) {
	    v.resize (st.st_size);
	    ssize_t vsz = 0;
	    while (vsz < st.st_size) {
		ssize_t br = read (fd, &v[vsz], st.st_size-vsz);
		if (br <= 0 && errno != EINTR) {
		    v.clear();
		    break;
		}
		vsz += br;
	    }
	}
    }
    close (fd);
    return v;
}

int WritePakbufToFd (const pakbuf_t& v, int fd) noexcept
{
    size_t tbw = 0;
    while (tbw < v.size()) {
	ssize_t bw = write (fd, &v[tbw], v.size()-tbw);
	if (bw <= 0 && errno != EINTR)
	    return -1;
	tbw += bw;
    }
    return 0;
}

int WritePakbufToFile (const pakbuf_t& v, const char* filename) noexcept
{
    int fd = open (filename, O_WRONLY| O_CREAT| O_TRUNC, DEFFILEMODE);
    if (fd < 0)
	return -1;
    int rv = WritePakbufToFd (v, fd);
    if (0 > close (fd))
	rv = -1;
    return rv;
}

//}}}-------------------------------------------------------------------
//{{{ Compression

enum { ZLIB_USE_GZIP_FORMAT = 16 };

pakbuf_t CompressPakbuf (const pakbuf_t& v)
{
    z_stream zs = {};
    deflateInit2 (&zs, Z_BEST_COMPRESSION, Z_DEFLATED, MAX_WBITS| ZLIB_USE_GZIP_FORMAT, 9, Z_DEFAULT_STRATEGY);

    zs.next_in = const_cast<uint8_t*>(&v[0]);	// zlib does not have const
    zs.avail_in = v.size();

    pakbuf_t r;
    r.resize (v.size());
    zs.next_out = &r[0];
    zs.avail_out = r.size();

    int ok;
    while (Z_OK == (ok = deflate (&zs, Z_FINISH))) {}

    if (ok != Z_STREAM_END)
	zs.avail_out = r.size();	// return empty vector on error

    r.resize (r.size()-zs.avail_out);
    r.shrink_to_fit();
    deflateEnd (&zs);
    return r;
}

pakbuf_t DecompressPakbuf (pakbuf_t::const_pointer buf, size_t bufsz)
{
    pakbuf_t r;

    z_stream zs = {};
    if (Z_OK != inflateInit2 (&zs, MAX_WBITS| ZLIB_USE_GZIP_FORMAT))
	return r;

    zs.next_in = const_cast<uint8_t*>(buf);	// zlib does not have const
    zs.avail_in = bufsz;
    auto rsz = bufsz;
    zs.avail_out = rsz;

    int ok;
    do {
	auto bw = rsz-zs.avail_out;
	r.resize (rsz*=2);
	zs.next_out = &r[bw];
	zs.avail_out = rsz-bw;
    } while (Z_OK == (ok = inflate (&zs, Z_NO_FLUSH)));

    if (ok != Z_STREAM_END)
	r.clear();
    else
	r.resize (r.size()-zs.avail_out);
    r.shrink_to_fit();

    inflateEnd (&zs);
    return r;
}

//}}}-------------------------------------------------------------------
//{{{ Member file access

void AddFileToPakbuf (pakbuf_t& pk, const char* filename, pakbuf_t::const_pointer fdata, size_t fdatasz)
{
    CpioHeader fh (filename, fdatasz);
    auto fhp = reinterpret_cast<pakbuf_t::const_pointer>(&fh);
    pk.insert (pk.end(), fhp, fhp+sizeof(fh));
    auto fnp = reinterpret_cast<pakbuf_t::const_pointer>(filename);
    pk.insert (pk.end(), fnp, fnp+fh.Namesize());
    if (fh.Namesize()%2)
	pk.push_back(0);
    if (fdatasz) {
	pk.insert (pk.end(), fdata, fdata+fdatasz);
	if (fdatasz%2)
	    pk.push_back(0);
    }
}

void AddFileToPakbuf (pakbuf_t& pk, const char* filename)
{
    AddFileToPakbuf (pk, filename, ReadFileToPakbuf (filename));
}

void FinishPakbuf (pakbuf_t& pk)
    { AddFileToPakbuf (pk, CPIO_TRAILER_FILENAME, NULL, 0); }

pakbuf_t::const_pointer FilePointerInPakbuf (const pakbuf_t& pk, const char* filename, size_t* fsz) noexcept
{
    pakbuf_t fdata;
    for (auto i = PakbufBegin(pk), iend = PakbufEnd(pk); i < iend; i = i->Next()) {
	// Get the filename
	auto ifn = i->Filename();
	if (reinterpret_cast<pakbuf_t::const_pointer>(ifn)+i->Namesize() > pk.end())
	    break;	// filename past the end
	if (ifn[i->Namesize()-1] != 0)
	    break;	// filename not 0-terminated
	if (0 != strcmp (filename, ifn))
	    continue;	// filename does not match
	// Copy the file data
	auto idata = i->Filedata();
	auto isize = i->Filesize();
	if (idata+isize > pk.end())
	    break;
	if (fsz)
	    *fsz = isize;
	return idata;
    }
    return NULL;
}

pakbuf_t ExtractFileFromPakbuf (const pakbuf_t& pk, const char* filename)
{
    size_t fsz = 0;
    auto pfile = FilePointerInPakbuf (pk, filename, &fsz);
    pakbuf_t fdata;
    if (pfile)
	fdata.assign (pfile, pfile+fsz);
    return fdata;
}

//}}}-------------------------------------------------------------------
