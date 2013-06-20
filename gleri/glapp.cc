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

void CGLApp::DeleteWindow (const CWindow* p)
{
    foreach (auto,w,_wins) {
	if (*w == p) {
	    (*w)->PostClose();
	    (*w)->WriteCmds();
	    delete (*w);
	    (*w) = nullptr;
	    --(w = _wins.erase(w));
	}
    }
    if (_wins.empty())
	Quit();
}

CWindow* CGLApp::ClientRecord (int fd, CWindow::wid_t wid)
{
    for (auto w : _wins)
	if (w->Matches (fd, wid))
	    return (w);
    return (nullptr);
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
    // Look for the GLERIS executable in PATH
    const char* pathenv = getenv("PATH");
    if (!pathenv)
	pathenv = "/bin:/usr/bin";
    char* srvexe = nullptr;
    for (char *pcolon, *path=strdup(pathenv), *pathend=path+strlen(path); path < pathend;) {
	if ((pcolon = strchr(path,':')))
	    *pcolon = 0;
	asprintf (&srvexe, "%s/" GLERIS_NAME, path);
	if (access (srvexe, X_OK) == 0)
	    break;
	free (srvexe);
	srvexe = nullptr;
	path += strlen(path)+1;
    }
    if (!srvexe)
	throw XError ("could not find " GLERIS_NAME " in PATH");
    // Server is launched with -s flag, in single application mode
    const char* srvcmd[] = { srvexe, "-s", nullptr };

    // Single app mode communicates through a socket on stdin
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
	if (0 > execve (srvcmd[0], const_cast<char* const*>(srvcmd), environ))
	    CFile::Error ("execve");
    }
    exit (EXIT_FAILURE);
}

void CGLApp::OnFd (int fd)
{
    CApp::OnFd (fd);
    if (fd != _srvbuf.Fd()) return;
    _srvbuf.ReadCmds();
    _srvbuf.ProcessMessages (*this, PRGLR::Parse<CGLApp>);
    for (auto w : _wins)
	w->WriteCmds();
}

void CGLApp::OnFdError (int fd)
{
    CApp::OnFdError (fd);
    if (fd != _srvbuf.Fd()) return;
    syslog (LOG_NOTICE, GLERIS_NAME " connection terminated");
    Quit();
}

void CGLApp::OnTimer (uint64_t tms)
{
    for (auto w : _wins) {
	w->OnTimer (tms);
	w->WriteCmds();
    }
}
