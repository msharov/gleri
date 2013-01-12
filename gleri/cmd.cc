// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "cmd.h"

//----------------------------------------------------------------------

/*static*/ inline const char* CCmdBuf::nextname (const char* n, size_type& sz) noexcept
{
#if __x86__
    asm("repnz\tscasb\n\trepnz\tscasb":"+D"(n),"+c"(sz):"a"(0):"memory");
    return (n);
#else
    const char* nn = n;
    nn += strlen(nn)+1;
    nn += strlen(nn)+1;
    sz -= nn - n;
    return (nn);
#endif
}

/*static*/ inline bool CCmdBuf::namecmp (const void* s1, const void* s2, size_type n) noexcept
{
#if __x86__
    asm("repz\tcmpsb":"+D"(s1),"+S"(s2),"+c"(n));
    return (!n);
#else
    return (!memcmp (s1,s2,n));
#endif
}

/*static*/ const char* CCmdBuf::LookupCmdName (unsigned cmd, size_type& sz, const char* cmdnames, size_type cleft) noexcept
{
    unsigned ci = InvalidCmd+1;
    for (const char *ns, *s = cmdnames; cleft; ++ci, s = ns) {
	ns = nextname(s,cleft);
	if (cmd == ci) {
	    sz = ns-s;
	    return (s);
	}
    }
    sz = 0;
    return (nullptr);
}

/*static*/ unsigned CCmdBuf::LookupCmd (const char* name, size_type bleft, const char* cmdnames, size_type cleft) noexcept
{
    unsigned ci = InvalidCmd+1, namesz = nextname(name,bleft)-name;
    for (const char* s = cmdnames; cleft; ++ci) {
	const char* ns = nextname(s,cleft);
	if (ns-s == namesz && namecmp(s,name,namesz))
	    return (ci);
	s = ns;
    }
    return (InvalidCmd);
}

inline CCmdBuf::size_type CCmdBuf::nextcapacity (size_type v) const noexcept
{
#if __x86__
    asm("bsr\t%0, %0":"+r"(v));
    return (1<<(1+v));
#else
    size_type r = 64;
    while (r <= v) r *= 2;	// Next power of 2
    return (r);
#endif
}

inline CCmdBuf::pointer CCmdBuf::addspace (size_type need) noexcept
{
    need += size();
    if (_sz < need)
	_buf = (pointer) realloc (_buf,_sz = nextcapacity(need));
    return (end());
}

bstro CCmdBuf::CreateCmd (const char* m, size_type msz, size_type sz) noexcept
{
    uint16_t fdoffset = m[msz-2] == 'h' ? sz-sizeof(int) : UINT16_MAX;
    uint8_t hsz = Align(sizeof(sz)+sizeof(_iid)+sizeof(fdoffset)+sizeof(hsz)+sizeof(RGLObject)+msz,8);
    const size_type cmdsz = hsz+(sz=Align(sz,8));
    pointer pip = addspace (cmdsz);
    bstro os (pip,cmdsz);
    os << sz << _iid << fdoffset << hsz << RGLObject;
    os.write (m, msz);
    os.align (8);
    _used += cmdsz;
    return (os);
}

void CCmdBuf::EndRead (bstri::const_pointer p) noexcept
{
    assert (p >= _buf && p <= _buf+_used && "EndRead must be given pointer returned by BeginRead");
    size_type br = p-_buf;
    memcpy (_buf, p, _used-=br);
}

void CCmdBuf::ReadCmds (void)
{
    if (!_outf.IsOpen()) return;
    size_t br;
    do {
	pointer pip = addspace (256);
	br = CanPassFd() ? _outf.ReadWithFdPass(pip, remaining()) : _outf.Read(pip, remaining());
	_used += br;
    } while (br);
}

void CCmdBuf::WriteCmds (void)
{
    if (!_outf.IsOpen()) return;
    bstri is (BeginRead());
    _outf.Write (is.ipos(), is.remaining());
    is.skip (is.remaining());
    EndRead (is);
}

void CCmdBuf::SendFile (CFile& f)
{
    WriteCmds();
    if (CanPassFd())
	_outf.SendFd (f);
    else {
	SSendFileHeader header;
	header.totalSize = header.sizeInThisPacket = f.Size(); header.startOffset = 0;
	_outf.Write (&header, sizeof(header));
	f.SendfileTo (_outf, header.sizeInThisPacket);
    }
}

bstri CCmdBuf::ReceiveFileOpen (bstri& is)
{
    const size_t hsize = CanPassFd() ? sizeof(int) : sizeof(SSendFileHeader);
    for (; is.remaining() < hsize; is = BeginRead()) {
	EndRead (is);
	ReadCmds();
    }
    if (CanPassFd()) {
	int fd;
	is >> fd;
	_recvf.Attach (fd);
    } else {
	SSendFileHeader header;
	is.read (&header, sizeof(header));
	if (header.totalSize < is.remaining())
	    return (bstri (is.ipos(), header.totalSize));
	if (!_recvf.IsOpen()) {
	    _recvf.Open();
	    _recvSize = header.totalSize;
	}
	_recvf.Write (is.ipos(), is.remaining());
	loff_t fsz = header.sizeInThisPacket-is.remaining();
	is.skip (is.remaining());
	_outf.CopyTo (_recvf, fsz);
    }
    _recvf.Map();
    if (CanPassFd())
	_recvSize = _recvf.MMSize();
    return (bstri (_recvf.MMData(), _recvf.MMSize()));
}

void CCmdBuf::ReceiveFileClose (void)
{
    if (ReceiveComplete()) {
	_recvf.Close();
	_recvSize = 0;
    } else
	_recvf.Unmap();
}
