// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "glapp.h"
#include <sys/socket.h>

CGLApp::~CGLApp (void) noexcept
{
    close (_srvsock);
}

void CGLApp::Init (argc_t argc, argv_t argv)
{
    CApp::Init (argc, argv);
    _srvpid = LaunchServer();
    WatchFd (_srvsock);
    _srvbuf.SetFd (_srvsock);
}

void CGLApp::OpenWindow (CWindow* w)
{
    w->OnInit();
    _wins.push_back (w);
    w->WriteCmds();
}

int CGLApp::LaunchServer (void) noexcept
{
    static const char* const c_srvcmd[] = { GLERIS_NAME, "-s", nullptr };

    int sock[2];
    if (0 > socketpair (PF_LOCAL, SOCK_STREAM| SOCK_NONBLOCK, 0, sock)) {
	perror ("socketpair");
	exit (EXIT_FAILURE);
    }

    pid_t cpid = fork();
    if (cpid < 0)
	perror ("fork");
    else if (cpid > 0) {
	_srvsock = sock[0];
	close (sock[1]);
	return (cpid);
    } else {
	close (sock[0]);
	dup2 (sock[1], 0);
	if (0 > execve (c_srvcmd[0], const_cast<char* const*>(c_srvcmd), environ))
	    perror ("execve");
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
