// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "mmfile.h"
#include "gldefs.h"
#include <sys/stat.h>
#if HAVE_SYS_SENDFILE_H
    #include <sys/sendfile.h>
#endif

//----------------------------------------------------------------------

void CFile::BindStream (const sockaddr* sa, socklen_t sasz, unsigned backlog)
{
    CreateSocket (sa->sa_family);
    int doreuse = 1;
    if (0 > setsockopt (_fd, SOL_SOCKET, SO_REUSEADDR, &doreuse, sizeof(doreuse)))
	Error ("setsockopt");
    if (0 > bind (_fd, sa, sasz))
	Error ("bind");
    if (0 > listen (_fd, backlog))
	Error ("listen");
}

bool CFile::ConnectStream (const sockaddr* sa, socklen_t sasz)
{
    CreateSocket (sa->sa_family);
    int crv;
    while (0 > (crv = connect (_fd, sa, sasz)) && errno == EINPROGRESS)
	WaitForWrite();
    if (crv >= 0)
	return (true);
    ForceClose();
    if (errno != ECONNREFUSED && errno != ENOENT)
	Error ("connect");
    return (false);
}

void CFile::Close (void)
{
    int r = close (_fd);
    if (r < 0)
	Error ("close");
    Detach();
}

size_t CFile::Read (void* d, size_t dsz)
{
    ssize_t br;
    while (0 >= (br = read (_fd, d, dsz))) {
	if (!br) {
	    close (_fd);
	    return (0);
	}
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
		WaitForWrite();
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
	if (!br)
	    WaitForRead();
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
    fdpassheader control_un;
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    // Get a pointer to the cm field and set as a SCM_RIGHTS message
    cmsghdr* cmptr = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_len = CMSG_LEN(sizeof(fdpasspayload_t));
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    *((fdpasspayload_t*) CMSG_DATA (cmptr)) = f.Fd();
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
    fdpassheader control_un;
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
	if (!br) {
	    close (_fd);
	    return (0);
	}
	if (errno == EINTR)
	    continue;
	if (errno == EAGAIN)
	    return (0);
	Error ("recvmsg");
    }

    cmsghdr* cmptr = CMSG_FIRSTHDR(&msg);
    if (cmptr && cmptr->cmsg_len == CMSG_LEN(sizeof(fdpasspayload_t))) {
	int fd = *((fdpasspayload_t*) CMSG_DATA (cmptr));
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

CTmpfile::~CTmpfile (void) noexcept
{
    Close();
}
