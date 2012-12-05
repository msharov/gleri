#include "rglp.h"
#include <unistd.h>

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
    pointer pip = addspace (sz+32);
    uint8_t hsz = 8+Align(1+sizeof(RGLObject)+msz,8);
    sz = Align(sz,8);
    const size_type cmdsz = hsz+sz;
    bstro os (pip,cmdsz);
    os << sz << _iid << hsz << RGLObject;
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

void CCmdBuf::ReadCmds (void) noexcept
{
    if (_fd < 0) return;
    pointer pip = addspace (256);
    ssize_t br = read (_fd, pip, remaining());
    if (br <= 0) {
	if (errno != EINTR && errno != EAGAIN)
	    perror ("srv read cmd");
	return;
    }
    _used += br;
}

void CCmdBuf::WriteCmds (void) noexcept
{
    if (_fd < 0) return;
    bstri is (BeginRead());
    while (is.remaining()) {
	ssize_t bw = write (_fd, is.ipos(), is.remaining());
	if (bw <= 0) {
	    if (errno != EINTR && errno != EAGAIN) {
		perror ("cmd write");
		break;
	    }
	}
	is.skip (bw);
    }
    EndRead (is);
}
