// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "util.h"

//----------------------------------------------------------------------

class bstrb {
public:
    using value_type	= unsigned char;
    using size_type	= unsigned int;
    using pointer	= value_type*;
    using const_pointer	= const value_type*;
    enum { is_sizing = false, is_reading = false, is_writing = false };
private:
    template <typename Stm, typename T, bool pod>
    struct object_streamer {
	static inline void	read (Stm& is, T& v) 		{ v.read(is); }
	static inline void	write (Stm& os, const T& v) 	{ v.write(os); }
    };
protected:
    inline size_type	wrstrlen (const char* s) const	{ return s?strlen(s):0; }
    inline constexpr size_type	align_size (size_type sz, size_type g) const	{ return (g-1)-((sz+(g-1))%g); }
    inline constexpr size_type	align_size (const_pointer p, size_type g) const	{ return align_size(p-(pointer)nullptr,g); }
    template <typename Stm, typename T>
    inline Stm&	read_object (Stm& is, T& v) { object_streamer<Stm,T,is_pod<T>::value>::read(is,v); return is; }
    template <typename Stm, typename T>
    inline Stm&	write_object (Stm& os, const T& v) { object_streamer<Stm,T,is_pod<T>::value>::write(os,v); return os; }
};

template <typename Stm, typename T>
struct bstrb::object_streamer<Stm,T,true> {
    static inline void	read (Stm& is, T& v) 		{ is.iread(v); }
    static inline void	write (Stm& os, const T& v) 	{ os.iwrite(v); }
};
template <typename Stm, typename T>
struct bstrb::object_streamer<Stm,vector<T>,false> {
    static inline void	read (Stm& is, vector<T>& v) 		{ uint32_t sz; is.iread(sz); v.resize(sz); if (!v.empty()) is.read(&v[0], sz*sizeof(T)); }
    static inline void	write (Stm& os, const vector<T>& v) 	{ os.iwrite(uint32_t(v.size())); if (!v.empty()) os.write (&v[0], v.size()*sizeof(T)); }
};

//----------------------------------------------------------------------

class bstrs : public bstrb {
public:
    enum { is_sizing = true };
public:
    inline		bstrs (void)		:_sz(0) { }
    inline pointer	ipos (void)		{ return nullptr; }
   inline const_pointer	ipos (void) const	{ return nullptr; }
    template <typename T>
    inline T*		iptr (void)		{ return nullptr; }
    inline size_type	remaining (void) const	{ return numeric_limits<size_type>::max(); }
    inline size_type	size (void) const	{ return _sz; }
    inline void		skip (size_type n)	{ _sz += n; }
    inline void		align (size_type g)	{ skip (align_size(size(),g)); }
    template <typename T>
    inline void		iwrite (const T& v)	{ skip(sizeof(v)); }
    inline void		write (const void*, size_type sz)	{ skip(sz); }
    inline void		write_strz (const char* v)		{ skip(wrstrlen(v)+1); }
    template <typename T>
    inline bstrs&	operator<< (const T& v)	{ return write_object (*this, v); }
    inline bstrs&	operator<< (const char* s);
private:
    size_type		_sz;
};

//----------------------------------------------------------------------

class bstro : public bstrb {
public:
    enum { is_writing = true };
public:
    inline		bstro (pointer p, size_type sz)	:_p(p),_pend(_p+sz) {}
    inline pointer	ipos (void)		{ return _p; }
   inline const_pointer	ipos (void) const	{ return _p; }
   inline const_pointer	end (void) const	{ return _pend; }
    template <typename T>
    inline T*		iptr (void)		{ return reinterpret_cast<T*>(ipos()); }
    inline size_type	remaining (void) const	{ return end()-ipos(); }
    inline size_type	size (void) const	{ return remaining(); }
    inline void		skip (size_type n)	{ assert(remaining()>=n && "stream overflow");  _p += n; }
    inline void		align (size_type g)	{ auto nz = align_size(ipos(),g); memset(ipos(),0,nz); skip(nz); }
    template <typename T>
    inline void		iwrite (const T& v)	{ *iptr<T>() = v; skip(sizeof(v)); }
    inline void		write (const void* v, size_type sz)	{ auto o = _p; skip(sz); memcpy (o,v,sz); }
    inline void		write_strz (const char* v)		{ write (v, strlen(v)+1); }
    template <typename T>
    inline bstro&	operator<< (const T& v)	{ return write_object (*this, v); }
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
   inline const_pointer	ipos (void) const	{ return _p; }
   inline const_pointer	end (void) const	{ return _pend; }
    template <typename T>
    inline const T*	iptr (void) const	{ return reinterpret_cast<const T*>(ipos()); }
    inline size_type	remaining (void) const	{ return end()-ipos(); }
    inline size_type	size (void) const	{ return remaining(); }
    inline void		iseek (const_pointer i)	{ assert(i <= end() && "stream underflow"); _p = i; }
    inline void		skip (size_type n)	{ iseek (ipos()+n); }
    inline void		align (size_type g)	{ skip (align_size(ipos(),g)); }
    template <typename T>
    inline void		iread (T& v)		{ v = *iptr<T>(); skip(sizeof(v)); }
    inline void		read (void* v, size_type sz)	{ assert(remaining()>=sz && "read overflow"); memcpy (v,ipos(),sz); skip(sz); }
    inline const char*	read_strz (void)	{ auto v = iptr<char>(); skip(strlen(v)+1); return ipos() <= end() ? v : nullptr; }
    template <typename T>
    inline bstri&	operator>> (T& v)	{ return read_object (*this, v); }
    inline bstri&	operator>> (const char*& s);
private:
    const_pointer	_p;
    const_pointer	_pend;
};

//----------------------------------------------------------------------

bstrs& bstrs::operator<< (const char* s)
{
    auto n = wrstrlen(s);
    if (n) ++n;
    skip (sizeof(size_type)+Align(n,4));
    return *this;
}

bstro& bstro::operator<< (const char* s)
{
    size_type n = wrstrlen(s);
    if (n) ++n;
    iwrite (n);
    write (s, n);
    align (4);
    return *this;
}

bstri& bstri::operator>> (const char*& s)
{
    size_type n;
    iread (n);
    n = Align (n, 4);
    s = iptr<char>();
    if (n > remaining())
	s = nullptr;
    else if (!n)
	--s;
    else
	skip (n);
    return *this;
}
