// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "mmfile.h"
#include "gldefs.h"
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/socket.h>
#if HAVE_SYS_SENDFILE_H
    #include <sys/sendfile.h>
#endif
#include <zlib.h>

//----------------------------------------------------------------------

void CFile::Open (const char* filename, int flags, mode_t mode)
{
    _fd = open (filename, flags, mode);
    if (_fd < 0)
	Error (filename);
}

void CFile::Close (void)
{
    int r = close (_fd);
    if (r < 0)
	Error ("close");
    _fd = -1;
}

size_t CFile::Read (void* d, size_t dsz)
{
    ssize_t br;
    while (0 > (br = read (_fd, d, dsz))) {
	if (errno == EAGAIN)
	    return (0);
	if (errno != EINTR)
	    Error ("read");
    }
    return (br);
}

void CFile::Write (const void* d, size_t dsz)
{
    const char* p = (const char*) d;
    while (dsz) {
	ssize_t bw = write (_fd, p, dsz);
	if (bw <= 0) {
	    if (errno == EAGAIN) {
		pollfd pfd = { _fd, POLLOUT, 0 };
		poll (&pfd, 1, -1);
		continue;
	    } else if (errno == EINTR)
		continue;
	    Error ("write");
	}
	dsz -= bw;
	p += bw;
    }
}

size_t CFile::Size (void) const
{
    struct stat st;
    if (0 > fstat (_fd, &st) || !S_ISREG(st.st_mode))
	Error ("stat");
    return (st.st_size);
}

void* CFile::Map (size_t dsz)
{
    void* p = mmap (nullptr, dsz, PROT_READ, MAP_PRIVATE, _fd, 0);
    if (p == MAP_FAILED)
	Error ("mmap");
    return (p);
}

void CFile::CopyTo (CFile& outf, size_t n)
{
    for (char buf [BUFSIZ]; n;) {
	size_t br = Read (buf, min(n,sizeof(buf)));
	if (!br) {
	    pollfd pfd = { _fd, POLLIN, 0 };
	    poll (&pfd, 1, -1);
	}
	outf.Write (buf, br);
	n -= br;
    }
}

#if HAVE_SYS_SENDFILE_H
void CFile::SendfileTo (CFile& outf, size_t n)
{
    while (n) {
	ssize_t bw = sendfile (outf.Fd(), _fd, nullptr, n);
	if (bw <= 0)
	    Error ("sendfile");
	n -= bw;
    }
}
#endif

void CFile::SendFd (CFile& f)
{
    msghdr msg;

    // Allocate a control message to hold one int
    union {
	cmsghdr cm;				// Header
	char control [CMSG_SPACE(sizeof(int))];	// Header+Payload
    } control_un;
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    // Get a pointer to the cm field and set as a SCM_RIGHTS message
    cmsghdr* cmptr = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_len = CMSG_LEN(sizeof(int));
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    *((int*) CMSG_DATA (cmptr)) = f.Fd();
    msg.msg_name = nullptr;
    msg.msg_namelen = 0;

    // File descriptors must be sent with some data, so send a zero int
    iovec iov;
    int zerodata = 0;
    iov.iov_base = &zerodata;
    iov.iov_len = sizeof(zerodata);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if (0 > sendmsg (_fd, &msg, 0))
	Error ("sendmsg");
}

size_t CFile::ReadWithFdPass (void* p, size_t psz)
{
    msghdr msg;

    // Set up control message buffer with space for one int
    union {
	cmsghdr cm;				// Header
	char control [CMSG_SPACE(sizeof(int))];	// Header+Payload
    } control_un;
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);
    msg.msg_name = nullptr;
    msg.msg_namelen = 0;

    // File descriptors must be sent with some data, so receive data
    iovec iov;
    iov.iov_base = p;
    iov.iov_len = psz;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    ssize_t br;
    while (0 >= (br = recvmsg (_fd, &msg, 0))) {
	if (errno == EINTR)
	    continue;
	if (errno == EAGAIN)
	    return (0);
	Error ("recvmsg");
    }

    cmsghdr* cmptr = CMSG_FIRSTHDR(&msg);
    if (cmptr && cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
	int fd = *((int*) CMSG_DATA (cmptr));
	if (fd < 0)
	    Error ("fdpass");
	*(int*)((char*)p+br-sizeof(int)) = fd;
    }
    return (br);
}

/*static*/ void CFile::Error (const char* op)
{
    throw XError ("%s: %s", op, strerror(errno));
}

//----------------------------------------------------------------------

/*static*/ CMMFile::pointer CMMFile::DecompressBlock (const_pointer p, size_type isz, size_type& osz)
{
    z_stream zs;
    memset (&zs, 0, sizeof(zs));

    zs.avail_in = isz;
    zs.next_in = const_cast<Bytef*>(p);
    unsigned bufsz = 4096, bread = 0;
    unsigned chunksz = bufsz;
    pointer out = (pointer) malloc (bufsz);
    zs.avail_out = bufsz;
    zs.next_out = out;

    enum { USE_GZIP_FORMAT = 16 };	// zlib fairy dust
    if (Z_OK != inflateInit2 (&zs, USE_GZIP_FORMAT+MAX_WBITS))
	Error ("gzip");

    for (;;) {
	int r = inflate (&zs, Z_NO_FLUSH);
	if (r == Z_STREAM_END)
	    break;
	else if (r == Z_OK) {
	    bread += chunksz-zs.avail_out;
	    out = (pointer) realloc (out, bufsz*2);
	    bufsz *= 2;
	    zs.next_out = out+bread;
	    zs.avail_out = chunksz = bufsz-bread;
	} else {
	    inflateEnd (&zs);
	    Error ("gzip");
	}
    }
    osz = bread + chunksz-zs.avail_out;
    inflateEnd (&zs);
    return (out);
}

//----------------------------------------------------------------------

CTmpfile::~CTmpfile (void)
{
    Close();
}
