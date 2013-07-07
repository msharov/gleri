// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "gldefs.h"
#include "bstr.h"
#include "mmfile.h"

//----------------------------------------------------------------------

class CCmd {
public:
    typedef uint32_t		size_type;
    typedef uint8_t		value_type;
    typedef uint16_t		iid_t;
    typedef uint32_t		cmd_t;
    typedef value_type*		pointer;
    typedef const value_type*	const_pointer;
    struct SMsgHeader {
	size_type	sz;
	iid_t		iid;
	uint8_t		fdoffset;
	uint8_t		hsz;
	uint32_t	objname;
    public:
	inline const char*	Cmdname (void) const	{ return ((const char*)this+sizeof(*this)); }
	inline const_pointer	Msgdata (void) const	{ return ((const_pointer)this+hsz); }
	inline size_type	Msgsize (void) const	{ return (hsz+sz); }
	inline bstri		Msgstrm (void) const	{ return (bstri (Msgdata(), sz)); }
    };
    struct SDataBlock {
	const void*	_p;
	size_type	_sz;
	inline		SDataBlock (void)			:_p(nullptr),_sz(0) {}
	inline		SDataBlock (const void* p, size_type sz):_p(p),_sz(sz) {}
	template <typename Stm>
	inline void write (Stm& os) const
	    { os << _sz; os.write(_p,_sz); os.align(sizeof(_sz)); }
	inline void read (bstri& is) {
	    is >> _sz; _p = is.ipos();
	    if (is.remaining() < _sz) _sz = 0;
	    is.skip (Align(_sz,sizeof(_sz)));
	}
    };
protected:
    enum : uint32_t { c_ObjectName = vpack4('C','O','M',0) };
    enum : cmd_t { InvalidCmd = numeric_limits<cmd_t>::max() };
    enum { c_MsgAlignment = 8 };
protected:
    static inline bstrs& variadic_arg_size (bstrs& ss)
	{ return (ss); }
    template <typename A, typename... Arg>
    static inline bstrs& variadic_arg_size (bstrs& ss, const A& a, const Arg&... args)
	{ ss << a; return (variadic_arg_size (ss, args...)); }

    static inline bstri& variadic_arg_read (bstri& is)
	{ return (is); }
    template <typename A, typename... Arg>
    static inline bstri& variadic_arg_read (bstri& is, A& a, Arg&... args)
	{ is >> a; return (variadic_arg_read (is, args...)); }

    template <typename Stm>
    static inline Stm& variadic_arg_write (Stm& os)
	{ return (os); }
    template <typename Stm, typename A, typename... Arg>
    static inline Stm& variadic_arg_write (Stm& os, const A& a, const Arg&... args)
	{ os << a; return (variadic_arg_write (os, args...)); }
};

//----------------------------------------------------------------------

#define GLERIS_EXPORTS		"RGL\0RGLR\0"

//----------------------------------------------------------------------

class CCmdBuf : public CCmd {
public:
    inline explicit		CCmdBuf (iid_t iid) noexcept	:_outf(),_iid(iid) {}
    inline			CCmdBuf (iid_t iid, int fd, bool fdpass)	:_outf(fd),_iid(iid),_bFdPass(fdpass) {}
    inline			~CCmdBuf (void) noexcept	{ if(_buf) free(_buf); _outf.Detach(); }
    inline iid_t		IId (void) const		{ return (_iid); }
    inline int			Fd (void) const			{ return (_outf.Fd()); }
    inline void			SetFd (int fd, bool fdPass = false)	{ _outf.Attach (fd); _bFdPass = fdPass; }
    inline bool			CanPassFd (void) const		{ return (_bFdPass); }
    inline size_type		size (void) const		{ return (_used); }
    inline size_type		capacity (void) const		{ return (_sz); }
    void			ForwardError (const char* m)	{ Cmd (ECmd::Error, m); }
    void			Export (const char* ol)		{ Cmd (ECmd::Export, ol); }
    void			ReadCmds (void);
    void			WriteCmds (void);
    inline bstri		BeginRead (void) const		{ return (bstri(_buf,_used)); }
    inline void			EndRead (const bstri& is)	{ EndRead(is.ipos()); }
    template <typename OT, typename PT>
    inline void			ProcessMessages (PT& pp);
protected:
    bstro			CreateCmd (uint32_t o, const char* m, size_type msz, size_type sz, size_type unwritten = 0) noexcept;
    void			SendFile (CFile& f, uint32_t fsz);
    static const char*		LookupCmdName (unsigned cmd, size_type& sz, const char* cmdnames, size_type cleft) noexcept;
    static unsigned		LookupCmd (const char* name, size_type bleft, const char* cmdnames, size_type cleft) noexcept;
    template <typename... Arg>
    static inline void		Args (bstri& is, Arg&... args);
private:
    enum class ECmd : cmd_t {	// COM interface
	Error,
	Export,
	NCmds
    };
private:
    template <typename... Arg>
    inline void			Cmd (ECmd cmd, const Arg&... args);
    template <typename F>
    static inline void		Parse (F& f, const SMsgHeader& h, CCmdBuf& cmdbuf);
    static inline const char*	LookupCmdName (ECmd cmd, size_type& sz) noexcept;
    static ECmd			LookupCmd (const char* name, size_type bleft) noexcept;
    bstro			CreateCmd (ECmd cmd, size_type sz, size_type unwritten = 0) noexcept;
private:
    static inline const char*	nextname (const char* n, size_type& sz) noexcept;
    static inline bool		namecmp (const void* s1, const void* s2, size_type n) noexcept;
    inline size_type		nextcapacity (size_type v) const noexcept;
    inline pointer		addspace (size_type need) noexcept;
    inline size_type		remaining (void) const	{ return (_sz-_used); }
    inline pointer		begin (void)		{ return (_buf); }
    inline pointer		end (void)		{ return (begin()+size()); }
    void			EndRead (bstri::const_pointer p) noexcept;
private:
    pointer			_buf	= nullptr;
    size_type			_sz	= 0;
    size_type			_used	= 0;
    CFile			_outf;
    iid_t			_iid;
    bool			_bFdPass= false;
    static const char		_cmdNames[];
};

//----------------------------------------------------------------------

template <typename... Arg>
inline void CCmdBuf::Cmd (ECmd cmd, const Arg&... args)
{
    bstrs ss;
    variadic_arg_size (ss, args...);
    bstro os = CreateCmd (cmd, ss.size());
    variadic_arg_write (os, args...);
}

template <typename OT, typename PT>
inline void CCmdBuf::ProcessMessages (PT& pp)
{
    bstri is = BeginRead();
    while (is.remaining() >sizeof(SMsgHeader)) {// While have commands
	const SMsgHeader& h = *is.iptr<SMsgHeader>();
	if (is.remaining() < h.Msgsize())
	    break;
	is.skip (h.Msgsize());
	try {
	    switch (h.objname) {
		case c_ObjectName:	Parse (pp, h, *this); break;
		case OT::c_ObjectName:	OT::Parse (pp, h, *this); break;
		default:		XError::emit ("no such object");
	    }
	} catch (XError& e) {
	    pp.ForwardError (h, e, Fd());
	}
    }
    EndRead(is);
}

//----------------------------------------------------------------------

template <typename... Arg>
/*static*/ inline void CCmdBuf::Args (bstri& is, Arg&... args)
{
    bstrs ss; variadic_arg_size (ss, args...);	// Size of args
    if (is.remaining() < ss.size())		// Have the whole thing?
	XError::emit ("RGL protocol error");	//  sz may be != ss.size if args has a string
    variadic_arg_read (is, args...);		// Read args
}

template <typename F>
/*static*/ inline void CCmdBuf::Parse (F& f, const SMsgHeader& h, CCmdBuf& cmdbuf)
{
    bstri cmdis (h.Msgstrm());
    switch (LookupCmd (h.Cmdname(), h.hsz)) {
	case ECmd::Error: 	{ const char* m = nullptr; Args(cmdis,m); XError::emit(m); } break;
	case ECmd::Export:	{ const char* m = nullptr; Args(cmdis,m); f.OnExport(m,cmdbuf.Fd()); } break;
	default:		XError::emit ("invalid protocol command");
    }
}
