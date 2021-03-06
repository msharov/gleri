// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "app.h"
#include <sys/wait.h>
#include <sys/time.h>

//{{{ Utility functions ------------------------------------------------

namespace {
static inline bool AtomicSet (bool* p)
{
#if __x86__
    bool o = true;
    asm ("xchg\t%0, %1":"+m"(*p),"+r"(o));
    return !o;
#elif __GNUC__ >= 4
    return !__sync_val_compare_and_swap (p, false, true);
#else
    const bool o (*p);
    *p = true;
    return !o;
#endif
}
static inline char _num_to_digit (uint8_t b)
{
    char d = (b & 0xF) + '0';
    return d <= '9' ? d : d+('A'-'0'-10);
}
static inline bool _printable (char c)
{
    return c >= 32 && c < 127;
}
} // namespace

//}}}-------------------------------------------------------------------
//{{{ hexdump

void hexdump (const void* pv, size_t n) noexcept
{
    auto p = reinterpret_cast<const uint8_t*>(pv);
    uint8_t line[65]; line[64] = '\n';
    for (size_t i = 0; i < n; i += 16) {
	memset (line, ' ', sizeof(line)-1);
	for (size_t h = 0; h < 16; ++h) {
	    if (i+h < n) {
		uint8_t b = p[i+h];
		line[h*3] = _num_to_digit(b>>4);
		line[h*3+1] = _num_to_digit(b);
		line[h+3*16] = _printable(b) ? b : '.';
	    }
	}
	write (STDOUT_FILENO, line, sizeof(line));
    }
}

//}}}-------------------------------------------------------------------

CApp* CApp::gs_pApp = nullptr;
int CApp::s_LastSignal = 0;

void CApp::TerminateHandler (void) noexcept // static
{
    alarm (1);
    syslog (LOG_ERR, "exiting on unexpected fatal error\n");
    exit (EXIT_FAILURE);
}

void CApp::SignalHandler (int sig) noexcept // static
{
    alarm (1);
    s_LastSignal = sig;
    if (!SigInSet(sig,sigset_Fatal))
	return;
    static bool doubleSignal = false;
    bool bFirst = AtomicSet (&doubleSignal);
    syslog (LOG_ERR, "signal: %s", strsignal(sig));
    #if USE_USTL && !defined(NDEBUG)
    if (isatty(STDERR_FILENO)) {
	CBacktrace bkt;
	cerr << bkt;
	cerr.flush();
    }
    #endif
    if (bFirst)
	exit (qc_ShellSignalQuitOffset+sig);
    _exit (qc_ShellSignalQuitOffset+sig);
}

int CApp::AckSignal (void) noexcept // static
{
    int r = 0;
    #if __x86__
	asm("xchg\t%0, %1":"+r"(r),"+m"(s_LastSignal));
    #else
	swap(r,s_LastSignal);
    #endif
    if (r) alarm(0);
    return r;
}

uint64_t CApp::NowMS (void) noexcept // static
{
    timeval tv;
    gettimeofday (&tv, NULL);
    return tv.tv_sec*1000+tv.tv_usec/1000;
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

void CApp::WaitForTime (uint64_t tms)
{
    auto i = _timer.begin();
    while (i < _timer.end() && *i >= tms)
	++i;		// _timer is reverse sorted
    if (i[-1] != tms)	// No duplicates
	_timer.emplace (i, tms);
}

bool CApp::CheckForQuitSignal (void) const
{
    auto sig = AckSignal();
    if (sig == SIGCHLD)
	waitpid(-1,nullptr,0);
    return SigInSet(sig,sigset_Quit);
}

void CApp::WaitForFdsAndTimers (void)
{
    auto timeToWait = _timer.back();
    auto now = NowMS();
    if (timeToWait != NoTimer)
	timeToWait = (timeToWait > now ? timeToWait - now : 0);
    auto prv = poll (&_watch[0], _watch.size(), uint32_t(timeToWait));
    if (prv < 0 && errno != EINTR)
	CFile::Error ("poll");
    for (auto i = 0u; i < _watch.size(); ++i) {
	auto efd = _watch[i].fd;
	auto rev = _watch[i].revents;
	if (rev & (POLLHUP| POLLNVAL)) {
	    _watch.erase (_watch.begin()+i--);
	    OnFdError (efd);
	} else if (rev & POLLIN)
	    OnFd (efd);
    }
    now = NowMS();
    for (uint64_t t; now >= (t = _timer.back());) {
	_timer.pop_back();
	OnTimer (t);
    }
}

int CApp::Run (void)
{
    while (!_quitting && !CheckForQuitSignal())
	WaitForFdsAndTimers();
    return EXIT_SUCCESS;
}
