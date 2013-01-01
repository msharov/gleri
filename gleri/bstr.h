#pragma once
#include "config.h"

//----------------------------------------------------------------------

class bstrb {
public:
    typedef unsigned char	value_type;
    typedef unsigned int	size_type;
    typedef value_type*		pointer;
    typedef const value_type*	const_pointer;
protected:
    inline constexpr size_type	align_size (size_type sz, size_type g) const	{ return ((g-1)-((sz+(g-1))%g)); }
    inline constexpr size_type	align_size (const_pointer p, size_type g) const	{ return (align_size(uintptr_t(p),g)); }
};

//----------------------------------------------------------------------

class bstrs : public bstrb {
private:
    template <typename T>
    inline T*		iptr (void)		{ return (nullptr); }
    inline size_type	wrstrlen (const char* s) const	{ return (s?strlen(s):0); }
public:
    inline		bstrs (void)		:_sz(0) { }
    inline size_type	remaining (void) const	{ return (UINT_MAX); }
    inline size_type	size (void) const	{ return (_sz); }
    inline void		skip (size_type n)	{ _sz += n; }
    inline void		align (size_type g)	{ skip (align_size(_sz,g)); }
    template <typename T>
    inline bstrs&	operator<< (const T& v)	{ skip(sizeof(v)); return (*this); }
    inline bstrs&	operator<< (const char* s);
    inline void		write (const void*, size_type sz)	{ skip(sz); }
    inline void		write_strz (const char* v)		{ skip(wrstrlen(v)+1); }
private:
    size_type		_sz;
};

//----------------------------------------------------------------------

class bstro : public bstrb {
private:
    template <typename T>
    inline T*		iptr (void)		{ return (reinterpret_cast<T*>(_p)); }
public:
    inline		bstro (pointer p, size_type sz)	:_p(p),_pend(_p+sz) {}
    inline pointer	ipos (void) const	{ return (_p); }
   inline const_pointer	end (void) const	{ return (_pend); }
    inline size_type	remaining (void) const	{ return (_pend-_p); }
    inline size_type	size (void) const	{ return (remaining()); }
    inline void		skip (size_type n)	{ assert(remaining()>=n && "stream overflow");  _p += n; }
    inline void		align (size_type g)	{ const size_type nz = align_size(_p,g); memset(_p,0,nz); skip(nz); }
    template <typename T>
    inline bstro&	operator<< (const T& v)	{ *iptr<T>() = v; skip(sizeof(v)); return (*this); }
    inline void		write (const void* v, size_type sz)	{ assert(_p+sz<=_pend && "write overflow"); memcpy (_p,v,sz); skip(sz); }
    inline void		write_strz (const char* v)		{ _p = (pointer) stpcpy((char*)_p,v)+1; assert(_p<=_pend && "write overflow"); }
    inline bstro&	operator<< (const char* s);
private:
    pointer		_p;
    const const_pointer	_pend;
};

//----------------------------------------------------------------------

class bstri : public bstrb {
private:
    template <typename T>
    inline const T*	iptr (void)		{ return (reinterpret_cast<const T*>(_p)); }
public:
    inline		bstri (const_pointer p, size_type sz)	:_p(p),_pend(_p+sz) {}
   inline const_pointer	ipos (void) const	{ return (_p); }
   inline const_pointer	end (void) const	{ return (_pend); }
    inline size_type	remaining (void) const	{ return (_pend-_p); }
    inline size_type	size (void) const	{ return (remaining()); }
    inline void		iseek (const_pointer i)	{ assert(_p <= end() && "stream underflow"); _p = i; }
    inline void		skip (size_type n)	{ assert(remaining()>=n && "stream underflow");  _p += n; }
    inline void		align (size_type g)	{ skip (align_size(_p,g)); }
    template <typename T>
    inline bstri&	operator>> (T& v)		{ v = *iptr<T>(); skip(sizeof(v)); return (*this); }
    inline void		read (void* v, size_type sz)	{ assert(remaining()>=sz && "read overflow"); memcpy (v,_p,sz); skip(sz); }
    inline const char*	read_strz (void)		{ const char* v = (const char*)_p; skip(strlen(v)+1); return (_p <= _pend ? v : nullptr); }
    inline bstri&	operator>> (const char*& s);
private:
    const_pointer	_p;
    const const_pointer	_pend;
};

//----------------------------------------------------------------------

inline bstrs& bstrs::operator<< (const char* s)
{
    skip (sizeof(size_type)+wrstrlen(s)+1);
    align(4);
    return (*this);
}

inline bstro& bstro::operator<< (const char* s)
{
    size_type sl = strlen(s);
    operator<< (sl);
    write (s,sl+1);
    align (4);
    return (*this);
}

inline bstri& bstri::operator>> (const char*& s)
{
    size_type sl;
    operator>> (sl);
    sl += align_size (++sl,4);
    s = remaining() < sl ? nullptr : (const char*)ipos();
    skip (sl);
    return (*this);
}
