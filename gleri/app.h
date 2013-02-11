// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "rglp.h"
#include "rglrp.h"
#include <sys/poll.h>
#include <signal.h>

class CApp {
public:
    typedef int			argc_t;
    typedef char* const*	argv_t;
public:
    inline		CApp (void);
    static inline CApp&	Instance (void)		{ assert (gs_pApp); return (*gs_pApp); }
    static inline const char* Name (void)	{ return (gs_Name); }
    static uint64_t	NowMS (void) noexcept;
    void		WaitForTime (uint64_t tms);
    inline void		Init (argc_t, argv_t argv)	{ gs_Name = argv[0]; }
    inline void		Quit (void) noexcept	{ _quitting = true; }
    int			Run (void);
protected:
    //{{{ Signal masks for signal handlers
    #define S(s) (1u<<(s))
    enum : uint32_t {
	sigset_Quit	= S(SIGINT)|S(SIGQUIT)|S(SIGTERM)|S(SIGPWR),
	sigset_Fatal	= S(SIGILL)|S(SIGABRT)|S(SIGBUS)|S(SIGFPE)|
			  S(SIGSYS)|S(SIGSEGV)|S(SIGALRM)|S(SIGXCPU),
	sigset_Msg	= S(SIGHUP)|S(SIGCHLD)|S(SIGWINCH)|S(SIGURG)|
			  S(SIGXFSZ)|S(SIGUSR1)|S(SIGUSR2)|S(SIGPIPE)
    };
    enum { qc_ShellSignalQuitOffset = 128 };
    #undef S
    //}}}
    static inline bool	SigInSet (int sig, uint32_t sset)	{ return ((1u<<sig)&sset); }
    static int		AckSignal (void) noexcept;
    void		WatchFd (int fd);
    void		StopWatchingFd (int fd) noexcept;
    inline virtual void	OnFd (int)		{ }
    inline virtual void	OnFdError (int)		{ }
    inline virtual void	OnTimer (uint64_t)	{ }
private:
    inline bool		CheckForQuitSignal (void) const;
    inline void		WaitForFdsAndTimers (void);
    static void		TerminateHandler (void) noexcept NORETURN;
    static void		SignalHandler (int sig) noexcept;
private:
    vector<pollfd>	_watch;
    vector<uint64_t>	_timer;
    bool		_quitting;
    static CApp*	gs_pApp;
    static const char*	gs_Name;
    static int		s_LastSignal;
};

//{{{ CApp implementation ----------------------------------------------

inline CApp::CApp (void)
:_watch()
,_timer(1,UINT64_MAX)
,_quitting(false)
{
    assert (!gs_pApp && "Application object must be a singleton");
    set_terminate (TerminateHandler);
    for (uint32_t i = NSIG; --i;)
	if (SigInSet (i, sigset_Quit| sigset_Fatal| sigset_Msg))
	    signal (i, SignalHandler);
    gs_pApp = this;
}

//}}}-------------------------------------------------------------------
//{{{ main

template <typename App>
inline int Tmain (typename App::argc_t argc, typename App::argv_t argv)
{
    int ec = EXIT_FAILURE;
    try {
	App& app (App::Instance());
	app.Init (argc, argv);
	ec = app.Run();
    } catch (XError& e) {
	fflush (stdout);
	fprintf (stderr, "%s: error: %s\n", argv[0], e.what());
    }
    return (ec);
}

#define GLERI_APP(app) \
    int main (CApp::argc_t argc, CApp::argv_t argv) \
    { return (Tmain<app>(argc,argv)); }

//}}}-------------------------------------------------------------------
