// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "mmfile.h"
#include "gldefs.h"
#include "bstr.h"
#include <sys/stat.h>
#if HAVE_SYS_SENDFILE_H
    #include <sys/sendfile.h>
#endif

//----------------------------------------------------------------------

void CFile::BindStream (const sockaddr* sa, socklen_t sasz, unsigned backlog)
{
    CreateSocket (sa->sa_family);
    auto doreuse = 1;
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
	return true;
    ForceClose();
    if (errno != ECONNREFUSED && errno != ENOENT)
	Error ("connect");
    return false;
}

// This accepts \p fd passed in by systemd socket activation if \p family matches
bool CFile::BindSystemdFd (int fd, sa_family_t family)
{
    // Already open, close before binding
    if (IsOpen())
	return false;
    int v;
    socklen_t l = sizeof(v);
    // The incoming socket must be a stream socket
    if (getsockopt (fd, SOL_SOCKET, SO_TYPE, &v, &l) < 0 || v != SOCK_STREAM)
	return false;
    // It must be a listening server socket
    if (getsockopt (fd, SOL_SOCKET, SO_ACCEPTCONN, &v, &l) < 0 || v != true)
	return false;
    // And it must match the family (PF_LOCAL or PF_INET)
    struct sockaddr_storage ss;
    l = sizeof(ss);
    if (getsockname(fd, (struct sockaddr*) &ss, &l) < 0 || ss.ss_family != (int) family)
	return false;
    // If matches, need to set the fd nonblocking for the poll loop to work
    auto f = fcntl (fd, F_GETFL);
    if (f < 0)
	return false;
    if (0 > fcntl (fd, F_SETFL, f| O_NONBLOCK| O_CLOEXEC))
	return false;
    Attach (fd);
    return true;
}

void CFile::Close (void)
{
    if (_fd >= 0 && 0 != close (_fd))
	Error ("close");
    Detach();
}

size_t CFile::Read (void* d, size_t dsz)
{
    ssize_t br;
    while (0 >= (br = read (_fd, d, dsz))) {
	if (!br) {
	    close (_fd);
	    return 0;
	}
	if (errno == EAGAIN)
	    return 0;
	if (errno != EINTR)
	    Error ("read");
    }
    return br;
}

void CFile::Write (const void* d, size_t dsz)
{
    auto p = (const char*) d;
    while (dsz) {
	auto bw = write (_fd, p, dsz);
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
    return st.st_size;
}

void* CFile::Map (size_t dsz)
{
    auto p = mmap (nullptr, dsz, PROT_READ, MAP_PRIVATE, _fd, 0);
    if (p == MAP_FAILED)
	Error ("mmap");
    return p;
}

void CFile::CopyTo (CFile& outf, size_t n)
{
    for (char buf [BUFSIZ]; n;) {
	auto br = Read (buf, min(n,sizeof(buf)));
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
	auto bw = sendfile (outf.Fd(), _fd, nullptr, n);
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
    memset (&control_un, 0, sizeof(control_un));
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
    // HACK: this works because GLERI has only one type of message that
    // passes an fd, where the message ends with an fd and is not 8-aligned,
    // requiring 4 bytes of padding. So, writing a uint64_t works here.
    iovec iov;
    uint64_t zerodata = 0;
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
    memset (&control_un, 0, sizeof(control_un));
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
	    return 0;
	}
	if (errno == EINTR)
	    continue;
	if (errno == EAGAIN)
	    return 0;
	Error ("recvmsg");
    }

    auto cmptr = CMSG_FIRSTHDR(&msg);
    if (cmptr && cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
	int fd = *((int*) CMSG_DATA (cmptr));
	if (fd < 0)
	    Error ("fdpass");
	*(int*)((char*)p+br-8) = fd;
    }
    return br;
}

void CFile::Error (const char* op) // static
{
    throw XError ("%s: %s", op, strerror(errno));
}

//----------------------------------------------------------------------

CTmpfile::~CTmpfile (void) noexcept
{
    Close();
}

//----------------------------------------------------------------------

#define XAUTH_NAME	"MIT-MAGIC-COOKIE-1"
enum { XAUTH_NAME_LEN = sizeof(XAUTH_NAME)-1 };
enum XauthFamily {
    XauthFamilyInternet,
    XauthFamilyLocal = 256
};

unsigned GetXauthData (const SXDisplay& dpy, char data [XAUTH_DATA_LEN])
{
    char filename [PATH_MAX];
    const char* xauthpath = getenv("XAUTHORITY");
    if (xauthpath)
	strcpy (filename, xauthpath);
    else
	snprintf (ArrayBlock(filename), "%s/.Xauthority", getenv("HOME"));
    if (access(filename,R_OK) != 0)
	return 0;
    CMMFile authfile (filename);
    bstri is (authfile.MMData(), authfile.MMSize());
    const uint16_t hostlen = strlen(dpy.host);
    while (is.remaining() >= 4) {
	uint16_t family, sz, match = 0;
	is >> family;
	match += (family == htons(XauthFamilyLocal));
	enum { str_Host, str_Display, str_AuthName, str_AuthData, str_Last };
	for (auto i = 0u; i < str_Last; ++i) {
	    is >> sz; sz = ntohs(sz);
	    if (is.remaining() < sz)
		return 0;
	    if (i == str_Host && sz == hostlen)
		match += !memcmp(is.iptr<char>(), dpy.host, hostlen);
	    else if (i == str_Display) {
		uint16_t dpynum = 0;
		for (auto d = is.iptr<char>(), e = d+sz; d < e;)
		    dpynum = dpynum*10 + (*d++ - '0');
		match += (dpynum == dpy.display);
	    } else if (i == str_AuthName && sz == XAUTH_NAME_LEN)
		match += !memcmp(is.iptr<char>(), XAUTH_NAME, XAUTH_NAME_LEN);
	    else if (i == str_AuthData && sz == XAUTH_DATA_LEN && match == 4) {
		memcpy (data, is.iptr<char>(), XAUTH_DATA_LEN);
		return sz;
	    }
	    is.skip (sz);
	}
    }
    return 0;
}

void ParseXDisplay (const char* dispstr, SXDisplay& dinfo)
{
    strncpy (dinfo.host, dispstr, sizeof(dinfo.host));
    dinfo.display = 0;
    dinfo.screen = 0;
    dinfo.host[sizeof(dinfo.host)-1] = 0;
    auto pdpynum = strchr (dinfo.host, ':');
    if (!pdpynum || pdpynum == dinfo.host) {
	gethostname (dinfo.host, sizeof(dinfo.host)-1);
	return;
    }
    *pdpynum++ = 0;
    auto pscrnum = strchr (pdpynum, ':');
    if (pscrnum) {
	*pscrnum++ = 0;
	dinfo.screen = atoi (pscrnum);
    }
    dinfo.display = atoi (pdpynum);
}
