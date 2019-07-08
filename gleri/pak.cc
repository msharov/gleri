#include "pak.h"
#include "mmfile.h"
#include "gldefs.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <zlib.h>

//{{{ File operations --------------------------------------------------

pakbuf_t ReadFileToPakbuf (const char* filename)
{
    CFile f (filename, O_RDONLY);
    auto st = f.Stat();
    if (!S_ISREG(st.st_mode) || st.st_size >= (1u<<23))
	throw XError ("invalid file '%s'", filename);
    pakbuf_t v (st.st_size);
    f.Read (&v[0], st.st_size);
    return v;
}

void WritePakbufToFd (const pakbuf_t& v, int fd)
{
    CFile f (fd);
    f.Write (&v[0], v.size());
    f.Detach();
}

void WritePakbufToFile (const pakbuf_t& v, const char* filename)
{
    CFile::CreateParentPath (filename);
    CFile f (filename, O_WRONLY| O_CREAT| O_TRUNC, DEFFILEMODE);
    f.Write (&v[0], v.size());
    f.Close();
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
	if (reinterpret_cast<pakbuf_t::const_pointer>(ifn)+i->Namesize() > &*pk.end())
	    break;	// filename past the end
	if (ifn[i->Namesize()-1] != 0)
	    break;	// filename not 0-terminated
	if (0 != strcmp (filename, ifn))
	    continue;	// filename does not match
	// Copy the file data
	auto idata = i->Filedata();
	auto isize = i->Filesize();
	if (idata+isize > &*pk.end())
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
