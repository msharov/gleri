// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "app.h"
#include <sys/wait.h>

//{{{ Utility functions ------------------------------------------------

namespace {
static inline bool AtomicSet (bool* p)
{
#if __x86__
    bool o (true);
    asm ("xchg\t%0, %1":"+m"(*p),"+r"(o));
    return (!o);
#elif __GNUC__ >= 4
    return (!__sync_val_compare_and_swap (p, false, true));
#else
    const bool o (*p);
    *p = true;
    return (!o);
#endif
}
static inline char _num_to_digit (uint8_t b)
{
    char d = (b & 0xF) + '0';
    return (d <= '9' ? d : d+('A'-'0'-10));
}
static inline bool _printable (char c)
{
    return (c >= 32 && c < 127);
}
} // namespace

//}}}-------------------------------------------------------------------
//{{{ hexdump

void hexdump (const void* pv, size_t n)
{
    const uint8_t* p = (const uint8_t*) pv;
    uint8_t line[65]; line[64] = '\n';
    for (size_t i = 0; i < n; i += 16) {
	for (size_t h = 0; h < 16; ++h) {
	    uint8_t b = (i+h) < n ? p[i+h] : 0;
	    line[h*3] = _num_to_digit(b>>4);
	    line[h*3+1] = _num_to_digit(b);
	    line[h*3+2] = ' ';
	    line[h+3*16] = _printable(b) ? b : '.';
	}
	write (STDOUT_FILENO, ArrayBlock(line));
    }
}

//}}}-------------------------------------------------------------------

/*static*/ CApp* CApp::gs_pApp = nullptr;
/*static*/ const char* CApp::gs_Name = nullptr;
/*static*/ int CApp::s_LastSignal = 0;

/*static*/ void CApp::TerminateHandler (void) noexcept
{
    alarm (1);
    fprintf (stderr, "%s exiting on unexpected fatal error\n", Name());
    exit (EXIT_FAILURE);
}

/*static*/ void CApp::SignalHandler (int sig) noexcept
{
    alarm (1);
    s_LastSignal = sig;
    if (!SigInSet(sig,sigset_Fatal))
	return;
    static bool doubleSignal = false;
    bool bFirst = AtomicSet (&doubleSignal);
    fprintf (stderr, "%s: signal: %s\n", Name(), strsignal(sig));
    if (bFirst)
	exit (qc_ShellSignalQuitOffset+sig);
    _exit (qc_ShellSignalQuitOffset+sig);
}

/*static*/ int CApp::AckSignal (void) noexcept
{
    int r = 0;
    #if __x86__
	asm("xchg\t%0, %1":"+r"(r),"+m"(s_LastSignal));
    #else
	swap(r,s_LastSignal);
    #endif
    if (r) alarm(0);
    return (r);
}

void CApp::WatchFd (int fd)
{
    _watch.push_back ((pollfd){fd,POLLIN,0});
}

void CApp::StopWatchingFd (int fd) noexcept
{
    for (auto i = _watch.begin(); i < _watch.end(); ++i)
	if (i->fd == fd)
	    --(i = _watch.erase(i));
}

int CApp::Run (void)
{
    while (!_quitting) {
	int sig = AckSignal();
	if (sig == SIGCHLD)
	    waitpid(-1,nullptr,0);
	else if (SigInSet(sig,sigset_Quit))
	    break;
	int prv = poll (&_watch[0], _watch.size(), -1);
	if (prv < 0 && errno != EINTR)
	    CFile::Error ("poll");
	for (unsigned i = 0; i < _watch.size(); ++i) {
	    int efd = _watch[i].fd;
	    uint16_t rev = _watch[i].revents;
	    if (rev & (POLLHUP| POLLNVAL)) {
		_watch.erase (_watch.begin()+i--);
		OnFdError (efd);
	    } else if (rev & POLLIN)
		OnFd (efd);
	}
    }
    return (EXIT_SUCCESS);
}
