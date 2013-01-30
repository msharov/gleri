// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "config.h"
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <fcntl.h>

//----------------------------------------------------------------------

class CFile {
public:
    enum { c_DefaultBacklog = 16 };
public:
    inline		CFile (void)		: _fd(-1) {}
    inline explicit	CFile (int fd)		: _fd(fd) {}
    inline		CFile (const char* filename, int flags, mode_t mode = 0);
    inline		~CFile (void)		{ close (_fd); }
    inline int		Fd (void) const		{ return (_fd); }
    inline bool		IsOpen (void) const	{ return (_fd >= 0); }
    inline void		Attach (int fd)		{ _fd = fd; }
    inline int		Detach (void)		{ int fd = _fd; _fd = -1; return (fd); }
    inline void		Open (const char* filename, int flags, mode_t mode = 0);
    inline void		CreateSocket (int domain);
    inline void		Bind (const char* sockpath, unsigned backlog = c_DefaultBacklog);
    inline void		Bind (uint32_t addr, uint16_t port, unsigned backlog = c_DefaultBacklog);
    inline bool		Connect (const char* sockpath);
    inline bool		Connect (uint32_t addr, uint16_t port);
    void		Close (void);
    inline void		ForceClose (void) noexcept	{ close (Detach()); }
    size_t		Size (void) const;
    size_t		Read (void* d, size_t dsz);
    void		Write (const void* d, size_t dsz);
    void*		Map (size_t dsz);
    void		Unmap (void* d, size_t dsz)	{ munmap (d, dsz); }
    void		CopyTo (CFile& outf, size_t n);
#if HAVE_SYS_SENDFILE_H
    void		SendfileTo (CFile& outf, size_t n);
#else
    inline void		SendfileTo (CFile& outf, size_t n)	{ CopyTo (outf, n); }
#endif
    void		SendFd (CFile& f);
    size_t		ReadWithFdPass (void* p, size_t psz);
    inline void		WaitForRead (void) const;
    inline void		WaitForWrite (void) const;
    static void		Error (const char* op) NORETURN;
protected:
    void		BindStream (const sockaddr* sa, socklen_t sasz, unsigned backlog = c_DefaultBacklog);
    bool		ConnectStream (const sockaddr* sa, socklen_t sasz);
private:
    int			_fd;
};

//----------------------------------------------------------------------

class CMMFile : public CFile {
public:
    typedef unsigned char	value_type;
    typedef unsigned		size_type;
    typedef value_type*		pointer;
    typedef const value_type*	const_pointer;
public:
    inline			CMMFile (void)			: CFile(),_sz(0),_p(nullptr) { }
    inline explicit		CMMFile (const char* filename)	: CFile(),_sz(0),_p(nullptr) { Open (filename); }
    inline			~CMMFile (void)			{ Unmap(); }
    inline void			Open (const char* filename)	{ CFile::Open (filename, O_RDONLY); Map(); }
    inline void			Close (void)			{ Unmap(); CFile::Close(); }
    inline void			Map (void)			{ _p = (pointer) CFile::Map (_sz = Size()); }
    inline void			Unmap (void)			{ CFile::Unmap (_p, _sz); }
    inline const_pointer	MMData (void) const		{ return (_p); }
    inline size_type		MMSize (void) const		{ return (_sz); }
    static pointer		DecompressBlock (const_pointer p, size_type isz, size_type& osz);
private:
    size_type			_sz;
    pointer			_p;
};

//----------------------------------------------------------------------

class CTmpfile : public CMMFile {
public:
    inline			CTmpfile (void)			: CMMFile(), _fp(nullptr) {}
				~CTmpfile (void);
    void			Open (void)			{ _fp = tmpfile(); if (!_fp) Error("tmpfile"); Attach (fileno(_fp)); }
    void			Close (void)			{ if (_fp) { Detach(); fclose (_fp); _fp = nullptr; } }
private:
    FILE*			_fp;
};

//----------------------------------------------------------------------

inline void CFile::Open (const char* filename, int flags, mode_t mode)
{
    if (0 > (_fd = open (filename, flags, mode)))
	Error (filename);
}

inline CFile::CFile (const char* filename, int flags, mode_t mode)
{
    Open (filename, flags, mode);
}

inline void CFile::WaitForRead (void) const
{
    pollfd pfd = { _fd, POLLIN, 0 };
    poll (&pfd, 1, -1);
}

inline void CFile::WaitForWrite (void) const
{
    pollfd pfd = { _fd, POLLOUT, 0 };
    poll (&pfd, 1, -1);
}

inline void CFile::CreateSocket (int domain)
{
    if (0 > (_fd = socket (domain, SOCK_STREAM| SOCK_NONBLOCK| SOCK_CLOEXEC, IPPROTO_IP)))
	Error ("socket");
}

inline void CFile::Bind (const char* sockpath, unsigned backlog)
{
    sockaddr_un sa;
    sa.sun_family = PF_LOCAL;
    strcpy (sa.sun_path, sockpath);
    BindStream ((const sockaddr*) &sa, sizeof(sa), backlog);
}

inline void CFile::Bind (uint32_t addr, uint16_t port, unsigned backlog)
{
    sockaddr_in sa;
    sa.sin_family = PF_INET;
    sa.sin_port = htons (port);
    sa.sin_addr.s_addr = htonl (addr);
    BindStream ((const sockaddr*) &sa, sizeof(sa), backlog);
}

inline bool CFile::Connect (const char* sockpath)
{
    sockaddr_un sa;
    sa.sun_family = PF_LOCAL;
    strcpy (sa.sun_path, sockpath);
    return (ConnectStream ((const sockaddr*) &sa, sizeof(sa)));
}

inline bool CFile::Connect (uint32_t addr, uint16_t port)
{
    sockaddr_in sa;
    sa.sin_family = PF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(addr);
    return (ConnectStream ((const sockaddr*) &sa, sizeof(sa)));
}
