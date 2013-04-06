// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "config.h"

//----------------------------------------------------------------------

class bstrb {
public:
    typedef unsigned char	value_type;
    typedef unsigned int	size_type;
    typedef value_type*		pointer;
    typedef const value_type*	const_pointer;
    enum { is_sizing = false, is_reading = false, is_writing = false };
protected:
    inline constexpr size_type	align_size (size_type sz, size_type g) const	{ return ((g-1)-((sz+(g-1))%g)); }
    inline constexpr size_type	align_size (const_pointer p, size_type g) const	{ return (align_size(p-(pointer)nullptr,g)); }
};

//----------------------------------------------------------------------

class bstrs : public bstrb {
public:
    enum { is_sizing = true };
private:
    inline size_type	wrstrlen (const char* s) const	{ return (s?strlen(s):0); }
public:
    inline		bstrs (void)		:_sz(0) { }
    template <typename T>
    inline T*		iptr (void)		{ return (nullptr); }
    inline size_type	remaining (void) const	{ return (UINT_MAX); }
    inline size_type	size (void) const	{ return (_sz); }
    inline void		skip (size_type n)	{ _sz += n; }
    inline void		align (size_type g)	{ skip (align_size(size(),g)); }
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
public:
    enum { is_writing = true };
public:
    inline		bstro (pointer p, size_type sz)	:_p(p),_pend(_p+sz) {}
    inline pointer	ipos (void)		{ return (_p); }
   inline const_pointer	ipos (void) const	{ return (_p); }
   inline const_pointer	end (void) const	{ return (_pend); }
    template <typename T>
    inline T*		iptr (void)		{ return (reinterpret_cast<T*>(ipos())); }
    inline size_type	remaining (void) const	{ return (end()-ipos()); }
    inline size_type	size (void) const	{ return (remaining()); }
    inline void		skip (size_type n)	{ assert(remaining()>=n && "stream overflow");  _p += n; }
    inline void		align (size_type g)	{ const size_type nz = align_size(ipos(),g); memset(ipos(),0,nz); skip(nz); }
    template <typename T>
    inline bstro&	operator<< (const T& v)	{ *iptr<T>() = v; skip(sizeof(v)); return (*this); }
    inline void		write (const void* v, size_type sz)	{ pointer o = _p; skip(sz); memcpy (o,v,sz); }
    inline void		write_strz (const char* v)		{ write (v, strlen(v)+1); }
    inline bstro&	operator<< (const char* s);
private:
    pointer		_p;
    const_pointer	_pend;
};

//----------------------------------------------------------------------

class bstri : public bstrb {
public:
    enum { is_reading = true };
public:
    inline		bstri (const_pointer p, size_type sz)	:_p(p),_pend(_p+sz) {}
   inline const_pointer	ipos (void) const	{ return (_p); }
   inline const_pointer	end (void) const	{ return (_pend); }
    template <typename T>
    inline const T*	iptr (void) const	{ return (reinterpret_cast<const T*>(ipos())); }
    inline size_type	remaining (void) const	{ return (end()-ipos()); }
    inline size_type	size (void) const	{ return (remaining()); }
    inline void		iseek (const_pointer i)	{ assert(i <= end() && "stream underflow"); _p = i; }
    inline void		skip (size_type n)	{ iseek (ipos()+n); }
    inline void		align (size_type g)	{ skip (align_size(ipos(),g)); }
    template <typename T>
    inline bstri&	operator>> (T& v)		{ v = *iptr<T>(); skip(sizeof(v)); return (*this); }
    inline void		read (void* v, size_type sz)	{ assert(remaining()>=sz && "read overflow"); memcpy (v,ipos(),sz); skip(sz); }
    inline const char*	read_strz (void)		{ const char* v = iptr<char>(); skip(strlen(v)+1); return (ipos() <= end() ? v : nullptr); }
    inline bstri&	operator>> (const char*& s);
private:
    const_pointer	_p;
    const_pointer	_pend;
};

//----------------------------------------------------------------------

inline bstrs& bstrs::operator<< (const char* s)
{
    skip (sizeof(size_type)+Align(wrstrlen(s)+1,4));
    return (*this);
}

inline bstro& bstro::operator<< (const char* s)
{
    size_type sl = strlen(s)+1;
    operator<< (sl);
    write (s,sl);
    align (4);
    return (*this);
}

inline bstri& bstri::operator>> (const char*& s)
{
    size_type sl;
    operator>> (sl);
    sl += align_size (sl,4);
    s = iptr<char>();
    if (sl > remaining())
	s = nullptr;
    else
	skip (sl);
    return (*this);
}
