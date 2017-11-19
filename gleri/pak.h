// This file is part of the GLERI project
//
// Copyright (c) 2017 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "util.h"

using pakbuf_t	= vector<uint8_t>;

//{{{ CPIO file format header ------------------------------------------

#define CPIO_TRAILER_FILENAME	"TRAILER!!!"

class CpioHeader {
public:
    enum { MAGIC = 070707 };
public:
		CpioHeader (const char* filename, size_t fsz)
		    :magic(MAGIC),dev(0),ino(0),mode(0100600),uid(0),gid(0),nlink(1),rdev(0),mtime{0}
		    ,namesize(strlen(filename)+1),filesize{uint16_t(fsz>>16),uint16_t(fsz)} {}
    inline auto	Filesize (void) const	{ return (uint32_t(filesize[0]) << 16) | filesize[1]; }
    inline auto	Namesize (void) const	{ return namesize; }
    inline auto	MagicOk (void) const	{ return magic == MAGIC; }
    inline auto	Filename (void) const	{ return reinterpret_cast<const char*>(this) + sizeof(*this); }
    inline auto	Filedata (void) const	{ return reinterpret_cast<pakbuf_t::const_pointer>(Filename()+Align(namesize,2)); }
    inline auto	Next (void) const	{ return reinterpret_cast<const CpioHeader*>(Filedata()+Align(Filesize(),2)); }
private:
    uint16_t	magic;		///< Magic id 070707
    uint16_t	dev;
    uint16_t	ino;
    uint16_t	mode;
    uint16_t	uid;
    uint16_t	gid;
    uint16_t	nlink;
    uint16_t	rdev;
    uint16_t	mtime[2];
    uint16_t	namesize;	///< Length of name right after the header, includes zero terminator. Name is padded to even offset.
    uint16_t	filesize[2];	///< Length of the file data after the name. Also padded to even offset.
};

//}}}-------------------------------------------------------------------
//{{{ Prototypes

pakbuf_t ReadFileToPakbuf (const char* filename);
void WritePakbufToFd (const pakbuf_t& v, int fd);
void WritePakbufToFile (const pakbuf_t& v, const char* filename);
pakbuf_t DecompressPakbuf (pakbuf_t::const_pointer buf, size_t bufsz);
pakbuf_t CompressPakbuf (const pakbuf_t& v);
void AddFileToPakbuf (pakbuf_t& pk, const char* filename);
void AddFileToPakbuf (pakbuf_t& pk, const char* filename, pakbuf_t::const_pointer fdata, size_t fdatasz);
void FinishPakbuf (pakbuf_t& pk);
pakbuf_t ExtractFileFromPakbuf (const pakbuf_t& pk, const char* filename);
pakbuf_t::const_pointer FilePointerInPakbuf (const pakbuf_t& pk, const char* filename, size_t* fsz) noexcept;

//}}}-------------------------------------------------------------------
//{{{ Inlines

static inline const CpioHeader* PakbufBegin (const pakbuf_t& pk) noexcept
    { return reinterpret_cast<const CpioHeader*>(&pk[0]); }
static inline CpioHeader* PakbufBegin (pakbuf_t& pk) noexcept
    { return reinterpret_cast<CpioHeader*>(&pk[0]); }
static inline const CpioHeader* PakbufEnd (const pakbuf_t& pk) noexcept
    { return reinterpret_cast<const CpioHeader*>(&pk[0]+pk.size()); }

static inline auto DecompressPakbuf (const pakbuf_t& v)
    { return DecompressPakbuf (&v[0], v.size()); }
static inline bool PakbufIsCompressed (const pakbuf_t& pk)
    { return pk[0] == 0x1f && pk[1] == 0x8b; } // Check for gzip header
static inline void AddFileToPakbuf (pakbuf_t& pk, const char* filename, const pakbuf_t& fdata)
    { AddFileToPakbuf (pk, filename, &fdata[0], fdata.size()); }

//}}}-------------------------------------------------------------------
