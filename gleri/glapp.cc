// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "glapp.h"

CGLApp::~CGLApp (void) noexcept
{
}

void CGLApp::Init (argc_t argc, argv_t argv)
{
    CApp::Init (argc, argv);
    ConnectToServer();
}

void CGLApp::OpenWindow (CWindow* w)
{
    w->SetFd (_srvsock.Fd(), _srvbuf.CanPassFd());
    w->OnInit();
    _wins.push_back (w);
    w->WriteCmds();
}

void CGLApp::ConnectToServer (void)
{
    // The X server must either be running locally or be ssh forwarded here
    // gleris requires a direct OpenGL context to run, and so a local server.
    // Remote connections should be forwarded through ssh for security and
    // performance, because gleri protocol is not encrypted or compressed.
    //
    const char *xdispstr = getenv("DISPLAY"), *xpcolon;
    if (!xdispstr || !(xpcolon = strchr (xdispstr, ':')))
	throw XError ("no locally accessible X11 server found");

    int idpy = atoi (xpcolon+1);
    bool bConnected, bCanPassFd = (xdispstr == xpcolon);
    if (bCanPassFd) {		// Try local server, PF_LOCAL socket
	char sockpath [sizeof(sockaddr_un::sun_path)];
	snprintf (ArrayBlock(sockpath), GLERIS_SOCKET, getenv("HOME"));
	bConnected = _srvsock.Connect (sockpath);
    } else {			// TCP socket, GLERIS_PORT+idpy port on localhost
	if (xpcolon-xdispstr != strlen("localhost") || memcmp(xdispstr,"localhost",strlen("localhost")))
	    throw XError ("no locally accessible X11 server found");
	bConnected = _srvsock.Connect (INADDR_LOOPBACK, GLERIS_PORT+idpy);
    }
    if (!bConnected) {		// Launch gleris in single mode if unable to connect
	_srvsock.Attach (LaunchServer());
	bCanPassFd = true;
    }
    _srvbuf.SetFd (_srvsock.Fd(), bCanPassFd);
    WatchFd (_srvsock.Fd());
}

/*static*/ int CGLApp::LaunchServer (void)
{
    static const char* const c_srvcmd[] = { GLERIS_NAME, "-s", nullptr };

    int sock[2];
    if (0 > socketpair (PF_LOCAL, SOCK_STREAM| SOCK_NONBLOCK, 0, sock))
	CFile::Error ("socketpair");

    pid_t cpid = fork();
    if (cpid < 0)
	CFile::Error ("fork");
    else if (cpid > 0) {
	close (sock[1]);
	return (sock[0]);
    } else {
	close (sock[0]);
	dup2 (sock[1], 0);
	if (0 > execve (c_srvcmd[0], const_cast<char* const*>(c_srvcmd), environ))
	    CFile::Error ("execve");
    }

    exit (EXIT_FAILURE);
}

void CGLApp::OnFd (int fd)
{
    if (fd != _srvbuf.Fd()) return;
    _srvbuf.ReadCmds();
    PRGLR::Parse (*_wins[0], _srvbuf);
    _wins[0]->WriteCmds();
}

void CGLApp::OnFdError (int fd)
{
    CApp::OnFdError (fd);
    printf ("gleris connection terminated\n");
    Quit();
}
