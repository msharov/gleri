// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "glapp.h"

CGLApp::~CGLApp (void) noexcept
{
}

void CGLApp::Init (argc_t argc, argv_t argv, EServerType stype)
{
    CApp::Init (argc, argv);
    _argc = argc;
    _argv = argv;
    ConnectToServer (stype);
}

void CGLApp::OpenWindow (CWindow* w)
{
    w->SetFd (_srvsock.Fd(), _srvbuf.CanPassFd());
    if (_wins.empty()) {
	w->Export ("");
	char hostname [HOST_NAME_MAX];
	gethostname (ArrayBlock(hostname));
	w->Authenticate (_argc, _argv, hostname, getpid(), 0, ArrayBlock(_xauth));
    }
    _wins.push_back (w);
    w->OnInit();
    w->WriteCmds();
}

CWindow* CGLApp::ClientRecord (int fd, CWindow::iid_t wid)
{
    for (auto w : _wins)
	if (w->Matches (fd, wid))
	    return w;
    return nullptr;
}

void CGLApp::ConnectToServer (EServerType stype)
{
    // Gleris must be either running locally or have its ports ssh forwarded.
    // When forwarded, set DISPLAY or GLERI_DISPLAY to the server hostname
    // and display. GLERI_DISPLAY has priority in case X is also forwarded,
    // in which case DISPLAY will contain local hostname instead of server.
    //
    const char* xdispstr = getenv("GLERI_DISPLAY");
    if (!xdispstr)
	xdispstr = getenv("DISPLAY");
    if (!xdispstr)
	XError::emit ("no locally accessible GLERI server found");
    SXDisplay dinfo;
    ParseXDisplay (xdispstr, dinfo);
    _screen = dinfo.screen;
    GetXauthData (dinfo, _xauth);

    bool bConnected = false, bCanPassFd = (xdispstr[0] == ':');
    if (bCanPassFd && (stype == server_Auto || stype == server_Local)) {	// Try local server, PF_LOCAL socket
	char sockpath [sizeof(sockaddr_un::sun_path)];
	auto sockfmt = GLERIS_XDG_SOCKET;
	const char* sockdir = getenv ("XDG_RUNTIME_DIR");
	if (!sockdir) {
	    sockfmt = GLERIS_SOCKET;
	    sockdir = getenv("HOME");
	}
	snprintf (ArrayBlock(sockpath), sockfmt, sockdir, dinfo.display);
	bConnected = _srvsock.Connect (sockpath);
    } else if (stype == server_Auto || stype == server_TCP)			// TCP socket, GLERIS_PORT+idpy port on localhost
	bConnected = _srvsock.Connect (INADDR_LOOPBACK, GLERIS_PORT+dinfo.display);
    if (!bConnected) {		// Launch gleris in single mode if unable to connect
	_srvsock.Attach (LaunchServer());
	bCanPassFd = true;
    }
    _srvbuf.SetFd (_srvsock.Fd(), bCanPassFd);
    WatchFd (_srvsock.Fd());
}

int CGLApp::LaunchServer (void) // static
{
    // Look for the GLERIS executable in PATH
    const char* pathenv = getenv("PATH");
    if (!pathenv)
	pathenv = "/bin:/usr/bin";
    char srvexe [PATH_MAX];
    unique_c_ptr<char> pathenvm (strdup(pathenv));
    for (char *pcolon, *path=pathenvm.get(), *pathend=path+strlen(path); path < pathend;) {
	if ((pcolon = strchr(path,':')))
	    *pcolon = 0;
	snprintf (ArrayBlock(srvexe), "%s/" GLERIS_NAME, path);
	if (access (srvexe, X_OK) == 0)
	    break;
	srvexe[0] = 0;
	path += strlen(path)+1;
    }
    if (!srvexe[0])
	XError::emit ("could not find " GLERIS_NAME " in PATH");
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
	return sock[0];
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
    _srvbuf.ProcessMessages<PRGLR> (*this);
    FinishWindowProcessing();
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
    for (auto w : _wins)
	w->OnTimer (tms);
    FinishWindowProcessing();
}

void CGLApp::FinishWindowProcessing (void)
{
    // Check for windows that asked to be deleted
    foreach (auto,w,_wins) {
	(*w)->WriteCmds();	// Write queued commands from all windows
	if ((*w)->DestroyPending()) {
	    delete (*w);
	    (*w) = nullptr;
	    --(w = _wins.erase(w));
	}
    }
    if (_wins.empty())
	Quit();
}

void CGLApp::SendUIEvent (const CEvent& e)
{
    for (auto w : _wins)
	w->OnEvent (e);
}
