// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "cmd.h"

//----------------------------------------------------------------------

const char* CCmdBuf::nextname (const char* n, size_type& sz) noexcept // static
{
#if __x86__
    asm("repnz\tscasb\n\trepnz\tscasb":"+D"(n),"+c"(sz):"a"(0):"memory");
    return n;
#else
    const char* nn = n;
    nn += strlen(nn)+1;
    nn += strlen(nn)+1;
    sz -= nn - n;
    return nn;
#endif
}

bool CCmdBuf::namecmp (const void* s1, const void* s2, size_type n) noexcept // static
{
#if __x86__
    asm("repz\tcmpsb":"+D"(s1),"+S"(s2),"+c"(n));
    return !n;
#else
    return !memcmp (s1,s2,n);
#endif
}

const char* CCmdBuf::LookupCmdName (unsigned cmd, size_type& sz, const char* cmdnames, size_type cleft) noexcept // static
{
    unsigned ci = InvalidCmd+1;
    for (const char *ns, *s = cmdnames; cleft; ++ci, s = ns) {
	ns = nextname(s,cleft);
	if (cmd == ci) {
	    sz = ns-s;
	    return s;
	}
    }
    sz = 0;
    assert (!"You have defined an ECmd enum, but not the command signature");
    exit (EXIT_FAILURE);
}

unsigned CCmdBuf::LookupCmd (const char* name, size_type bleft, const char* cmdnames, size_type cleft) noexcept // static
{
    unsigned ci = InvalidCmd+1, namesz = nextname(name,bleft)-name;
    for (const char* s = cmdnames; cleft; ++ci) {
	const char* ns = nextname(s,cleft);
	if (ns-s == namesz && namecmp(s,name,namesz))
	    return ci;
	s = ns;
    }
    return InvalidCmd;
}

CCmdBuf::size_type CCmdBuf::nextcapacity (size_type v) const noexcept
{
#if __x86__
    asm("bsr\t%0, %0":"+r"(v));
    return 1<<(1+v);
#else
    size_type r = 64;
    while (r <= v) r *= 2;	// Next power of 2
    return r;
#endif
}

CCmdBuf::pointer CCmdBuf::addspace (size_type need) noexcept
{
    need += size();
    if (_sz < need) {
	auto nsz = nextcapacity (need);
	_buf = (pointer) realloc (_buf, nsz);
	memset (_buf+_sz, 0, nsz-_sz);
	_sz = nsz;
    }
    return end();
}

bstro CCmdBuf::CreateCmd (uint32_t o, const char* m, size_type msz, size_type sz, size_type unwritten) noexcept
{
    assert (!unwritten || sz-unwritten == Align(sz-unwritten,c_MsgAlignment));
    SMsgHeader h = {
	Align(sz,c_MsgAlignment),
	_iid,
	(uint8_t) (m[msz-2] == 'h' ? sz-sizeof(int) : UINT8_MAX),
	(uint8_t) Align(sizeof(SMsgHeader)+msz,c_MsgAlignment),
	o
    };
    const size_type cmdsz = h.hsz+Align(sz-unwritten,c_MsgAlignment);
    pointer pip = addspace (cmdsz);
    bstro os (pip,cmdsz);
    os << h;
    os.write (m, msz);
    os.align (c_MsgAlignment);
    _used += cmdsz;
    return os;
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

void CCmdBuf::SendFile (CFile& f, uint32_t fsz)
{
    WriteCmds();
    if (CanPassFd())
	_outf.SendFd (f);
    else {
	_outf.Write (&fsz, sizeof(fsz));
	f.SendfileTo (_outf, fsz);
	uint64_t zeropad = 0;
	uint32_t alignedSize = Align(sizeof(fsz)+fsz, c_MsgAlignment);
	_outf.Write (&zeropad, alignedSize-(sizeof(fsz)+fsz));
    }
}

//----------------------------------------------------------------------
// COM object interface

#define N(n,s)	#n "\0" #s "\0"
const char CCmdBuf::_cmdNames[] =
     N(error,s)
     N(export,s)
     N(delete,)
;
#undef N

const char* CCmdBuf::LookupCmdName (ECmd cmd, size_type& sz) noexcept // static
{
    return CCmdBuf::LookupCmdName((unsigned)cmd,sz,ArrayBlock(_cmdNames)-1);
}

CCmdBuf::ECmd CCmdBuf::LookupCmd (const char* name, size_type bleft) noexcept // static
{
    return ECmd(CCmdBuf::LookupCmd(name,bleft,ArrayBlock(_cmdNames)-1));
}

bstro CCmdBuf::CreateCmd (ECmd cmd, size_type sz, size_type unwritten) noexcept
{
    size_type msz;
    const char* m = LookupCmdName (cmd, msz);
    return CCmdBuf::CreateCmd (c_ObjectName, m, msz, sz, unwritten);
}
