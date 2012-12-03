#pragma once
#include "config.h"
#include <sys/poll.h>
#include <signal.h>
#include <assert.h>

class CApp {
public:
    typedef int			argc_t;
    typedef char* const*	argv_t;
public:
    inline		CApp (void);
    inline virtual	~CApp (void) noexcept	{ }
    static inline CApp&	Instance (void)		{ assert (gs_pApp); return (*gs_pApp); }
    inline void		Init (argc_t, argv_t)	{ }
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
    inline virtual void	OnFd (int)		{ }
    inline virtual void	OnFdError (int)		{ }
private:
    static void		TerminateHandler (void) noexcept NORETURN;
    static void		SignalHandler (int sig) noexcept;
private:
    vector<pollfd>	_watch;
    bool		_quitting;
    static CApp*	gs_pApp;
    static int		s_LastSignal;
};

//{{{ CApp implementation ----------------------------------------------

inline CApp::CApp (void)
:_watch()
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
//{{{ XError

class XError {
public:
    template <typename... T>
    inline explicit	XError (const char* fmt, T... args) noexcept __attribute__((__format__(__printf__,1,2)));
    inline		~XError (void) noexcept		{ free (_msg); }
    inline const char*	what (void) const noexcept	{ return (_msg); }
private:
    char*		_msg;
};

template <typename... T>
inline XError::XError (const char* fmt, T... args) noexcept
    { asprintf (&_msg, fmt, args...); }

void hexdump (const void* pv, size_t n);

//}}}-------------------------------------------------------------------
//{{{ main

template <typename App>
int Tmain (typename App::argc_t argc, typename App::argv_t argv)
{
    int ec = EXIT_FAILURE;
    try {
	App& app (App::Instance());
	app.Init (argc, argv);
	ec = app.Run();
    } catch (XError& e) {
	printf ("Error: %s\n", e.what());
    }
    return (ec);
}

#define GLERI_APP(app) \
    int main (CApp::argc_t argc, CApp::argv_t argv) \
    { return (Tmain<app>(argc,argv)); }

//}}}-------------------------------------------------------------------
