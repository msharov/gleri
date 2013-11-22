// This file is part of the GLERI project
//
// Copyright (c) 2013 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "widget.h"

class CPackbox : public CWidget {
    typedef vector<CWidget*>		wigvec_t;
public:
    typedef wigvec_t::size_type		size_type;
    typedef dereferencing_iterator<wigvec_t::iterator> iterator;
    typedef dereferencing_iterator<wigvec_t::const_iterator> const_iterator;
    typedef iterator::value_type	value_type;
    typedef iterator::reference		reference;
    typedef iterator::const_reference	const_reference;
    typedef iterator::pointer		pointer;
    typedef iterator::const_pointer	const_pointer;
public:
    inline			CPackbox (PRGL* prgl)		: CWidget(prgl), _wigv() {}
    virtual			~CPackbox (void);
    inline size_type		size (void) const		{ return (_wigv.size()); }
    inline size_type		capacity (void) const		{ return (_wigv.capacity()); }
    inline bool			empty (void) const		{ return (_wigv.empty()); }
    inline iterator		begin (void)			{ return (_wigv.begin()); }
    inline const_iterator	begin (void) const		{ return (_wigv.begin()); }
    inline iterator		end (void)			{ return (_wigv.end()); }
    inline const_iterator	end (void) const		{ return (_wigv.end()); }
    inline iterator		iat (size_type i)		{ return (_wigv.begin()+i); }
    inline const_iterator	iat (size_type i) const		{ return (_wigv.begin()+i); }
    inline reference		operator[] (size_type i)	{ return (*_wigv[i]); }
    inline const_reference	operator[] (size_type i) const	{ return (*_wigv[i]); }
    inline void			reserve (size_type n)		{ _wigv.reserve (n); }
    inline void			shrink_to_fit (void)		{ _wigv.shrink_to_fit(); }
    iterator			erase (iterator f, iterator l) noexcept;
    inline iterator		erase (iterator ep)		{ return (erase (ep,ep+1)); }
    inline void			clear (void)			{ erase (begin(), end()); }
    iterator			FindEnclosing (coord_t x, coord_t y) noexcept;
    size_type			Focus (void) const noexcept;
    void			SetFocus (size_type f) noexcept;
    inline void			push_back (pointer&& v)				{ _wigv.push_back (forward<pointer>(v)); }
    inline iterator		insert (iterator ip, pointer v)			{ return (_wigv.insert (ip.base(), v)); }
    inline iterator		insert (iterator ip, pointer&& v)		{ return (_wigv.insert (ip.base(), forward<pointer>(v))); }
    template <typename W, typename... Args>
    inline iterator		emplace (iterator ip, Args&&... args)		{ return (insert (ip.base(), CreateSubWidget<W,Args...>(forward<Args>(args)...))); }
    template <typename W, typename... Args>
    inline void			emplace_back (Args&&... args)			{ push_back (CreateSubWidget<W,Args...>(forward<Args>(args)...)); }
    virtual void		OnResize (dim_t w, dim_t h);
    ONWIGDRAWDECL		OnDraw (Drw& drw) const;
    virtual SSize		OnMeasure (void) const;
    virtual void		OnEvent (const CEvent& e);
private:
    wigvec_t			_wigv;
};
