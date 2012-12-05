#pragma once
#include "gldefs.h"
#include "bstr.h"

//----------------------------------------------------------------------

class CCmd {
public:
    typedef uint32_t		size_type;
    typedef uint8_t		value_type;
    typedef uint16_t		iid_t;
    typedef value_type*		pointer;
    typedef const value_type*	const_pointer;
protected:
    enum : uint32_t { RGLObject = RGBA('R','G','L',0) };
    enum : unsigned { InvalidCmd = UINT_MAX };
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
public:
    struct SDataBlock {
	const void*	_p;
	size_type	_sz;
	inline		SDataBlock (const void* p, size_type sz) :_p(p),_sz(sz) {}
    };
};

//----------------------------------------------------------------------

class CCmdBuf : public CCmd {
public:
    inline			CCmdBuf (int fd, iid_t iid)	:_buf(nullptr),_sz(0),_used(0),_iid(iid),_fd(fd) {}
    inline			~CCmdBuf (void)		{ if(_buf) free(_buf); }
    inline iid_t		IId (void) const	{ return (_iid); }
    inline int			Fd (void) const		{ return (_fd); }
    inline void			SetFd (int fd)		{ _fd = fd; }
    inline size_type		size (void) const	{ return (_used); }
    inline size_type		capacity (void) const	{ return (_sz); }
    void			ReadCmds (void) noexcept;
    void			WriteCmds (void) noexcept;
    inline bstri		BeginRead (void) const	{ return (bstri(_buf,_used)); }
    inline void			EndRead (const bstri& is)	{ EndRead(is.ipos()); }
protected:
    bstro			CreateCmd (const char* m, size_type msz, size_type sz) noexcept;
    static const char*		LookupCmdName (unsigned cmd, size_type& sz, const char* cmdnames, size_type cleft) noexcept;
    static unsigned		LookupCmd (const char* name, size_type bleft, const char* cmdnames, size_type cleft) noexcept;
    static inline void		Error (void)		{ throw XError ("protocol error"); }
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
    iid_t			_iid;
    short			_fd;
};

//----------------------------------------------------------------------

inline bstrs& operator<< (bstrs& ss, const CCmd::SDataBlock& b)
    { ss << b._sz; ss.write(b._p,b._sz); ss.align(sizeof(b._sz)); return(ss); }
inline bstro& operator<< (bstro& os, const CCmd::SDataBlock& b)
    { os << b._sz; os.write(b._p,b._sz); os.align(sizeof(b._sz)); return(os); }

//----------------------------------------------------------------------
