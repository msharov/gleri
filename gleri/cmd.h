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
    };
    struct SDataBlock {
	const void*	_p;
	size_type	_sz;
	inline		SDataBlock (void)			:_p(nullptr),_sz(0) {}
	inline		SDataBlock (const void* p, size_type sz):_p(p),_sz(sz) {}
    };
protected:
    enum : uint32_t { COMObject = RGBA('C','O','M',0) };
    enum : cmd_t { InvalidCmd = UINT_MAX };
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

class CCmdBuf : public CCmd {
public:
    inline explicit		CCmdBuf (iid_t iid=0) noexcept	:_buf(nullptr),_sz(0),_used(0),_outf(),_iid(iid),_bFdPass(false) {}
    inline			~CCmdBuf (void) noexcept	{ if(_buf) free(_buf); _outf.Detach(); }
    inline iid_t		IId (void) const		{ return (_iid); }
    inline int			Fd (void) const			{ return (_outf.Fd()); }
    inline void			SetFd (int fd, bool fdPass = false)	{ _outf.Attach (fd); _bFdPass = fdPass; }
    inline bool			CanPassFd (void) const		{ return (_bFdPass); }
    inline size_type		size (void) const		{ return (_used); }
    inline size_type		capacity (void) const		{ return (_sz); }
    void			ForwardError (const char* m);
    void			ReadCmds (void);
    void			WriteCmds (void);
    inline bstri		BeginRead (void) const		{ return (bstri(_buf,_used)); }
    inline void			EndRead (const bstri& is)	{ EndRead(is.ipos()); }
    template <typename T, typename MProc>
    inline void			ProcessMessages (T& o, MProc f);
protected:
    bstro			CreateCmd (uint32_t o, const char* m, size_type msz, size_type sz, size_type unwritten = 0) noexcept;
    void			SendFile (CFile& f, uint32_t fsz);
    static const char*		LookupCmdName (unsigned cmd, size_type& sz, const char* cmdnames, size_type cleft) noexcept;
    static unsigned		LookupCmd (const char* name, size_type bleft, const char* cmdnames, size_type cleft) noexcept;
    static void			Error (void)			{ throw XError ("protocol error"); }
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
    pointer			_buf;
    size_type			_sz;
    size_type			_used;
    CFile			_outf;
    iid_t			_iid;
    bool			_bFdPass;
};

//----------------------------------------------------------------------

inline bstrs& operator<< (bstrs& ss, const CCmd::SDataBlock& b)
    { ss << b._sz; ss.write(b._p,b._sz); ss.align(sizeof(b._sz)); return(ss); }
inline bstro& operator<< (bstro& os, const CCmd::SDataBlock& b)
    { os << b._sz; os.write(b._p,b._sz); os.align(sizeof(b._sz)); return(os); }
inline bstri& operator>> (bstri& is, CCmd::SDataBlock& b)
{
    uint32_t sz;
    const void* p;
    is >> sz;
    p = is.ipos();
    if (is.remaining() < sz)
	sz = 0;
    is.skip (Align(sz,sizeof(sz)));
    b._sz = sz; b._p = p;
    return (is);
}

//----------------------------------------------------------------------

template <typename T, typename MProc>
inline void CCmdBuf::ProcessMessages (T& o, MProc f)
{
    bstri is = BeginRead();
    while (is.remaining() >sizeof(SMsgHeader)) {// While have commands
	const SMsgHeader& h = *is.iptr<SMsgHeader>();
	unsigned btodata = h.hsz-sizeof(SMsgHeader);
	if (is.remaining() < btodata+h.sz)
	    break;
	is.skip (sizeof(SMsgHeader));
	const char* cmdname = (const char*) is.ipos();
	is.skip (btodata);
	bstri cmdis (is.ipos(), h.sz);		// Command data stream
	is.skip (h.sz);				// Skip to next command
	f (o, h, cmdname, *this, cmdis);
    }
    EndRead(is);
}
//----------------------------------------------------------------------
