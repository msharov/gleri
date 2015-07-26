// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gldefs.h"

namespace G {

//----------------------------------------------------------------------

namespace Font {

CPMap::iterator& CPMap::iterator::operator++ (void) noexcept
{
    if (_v == UINT16_MAX)
	return *this;
    ++_v;
    uint8_t cpi = _v >> 8, cpo = _v;
    do {
	auto& r = _cpra[cpi];
	if (cpo < r.first)
	    cpo = r.first;
	if (cpo >= r.first + r.n) {
	    if (cpi == UINT8_MAX)
		cpo = UINT8_MAX;
	    else {
		cpo = 0;
		++cpi;
		continue;
	    }
	}
    } while (false);
    _v = (cpi << 8)|cpo;
    return *this;
}

CPMap::iterator CPMap::begin (void) const noexcept
{
    auto i = 0u;
    while (i < ArraySize(_cpra) && !_cpra[i].n)
	++i;
    return i == ArraySize(_cpra) ? end() : iterator (_cpra, (i << 8) + _cpra[i].first);
}

CPMap::iterator CPMap::end (void) const noexcept
{
    return iterator (_cpra, UINT16_MAX);
}

size_t CPMap::size (void) const noexcept
{
    auto i = ArraySize(_cpra);
    while (--i && !_cpra[i].n) {}
    return _cpra[i].offset + _cpra[i].n;
}

void CPMap::Create (const charmap_t& cm) noexcept
{
    uint16_t offset = 1;
    for (auto cp = 0u; cp < ArraySize(_cpra); ++cp) {
	auto& r = _cpra[cp];
	const auto cmf = &cm[cp*256];
	auto i = 0u;
	while (i < 256 && !cmf[i]) ++i;
	if (i >= 256)
	    continue;
	r.first = i;
	auto j = 256u;
	while (--j && !cmf[j]) {}
	r.n = j - i + 1;
	r.offset = offset;
	offset += r.n;
    }
}

uint16_t CPMap::operator[] (uint16_t i) const noexcept
{
    uint8_t cpi = i >> 8, cpo = i;
    auto& r = _cpra[cpi];
    cpo -= r.first;
    return cpo < r.n ? r.offset + cpo : 0;
}

//----------------------------------------------------------------------

Info::Info (void)
:_w (0)
,_h (0)
,_b (0)
,_mw (0)
,_mh (0)
,_style (Style::Regular)
,_cpmap()
,_name()
,_varw()
,_kp()
{
}

Info::Info (dim_t w, dim_t h)
:_w (w)
,_h (h)
,_b (0)
,_mw (w)
,_mh (h)
,_style (Style::Regular)
,_cpmap()
,_name()
,_varw()
,_kp()
{
}

dim_t Info::Width (uint16_t c) const noexcept
{
    return IsFixed() ? Width() : _varw[_cpmap[c]];
}

dim_t Info::Width (const char* s) const noexcept
{
    dim_t w = 0;
    const auto send = s+strlen(s);
    uint16_t prevc = 0;
    for (auto i = utf8in(s), iend = utf8in(send); i < iend; prevc = *i++)
	w += Width(*i) + Kerning (prevc, *i);
    return w;
}

int16_t Info::Kerning (uint16_t c1, uint16_t c2) const noexcept
{
    if (!HasKerning())
	return 0;
    const KerningPair m = { 0, 0, c2, c1 };
    auto f = lower_bound (_kp.begin(), _kp.end(), m);
    if (f < _kp.end() && f->c1 == c1 && f->c2 == c2)
	return f->d;
    return 0;
}

void Info::read (bstri& is)
{
    uint16_t nvarw, nkp;
    is >> _w >> _h >> _b >> _mw >> _mh >> _style >> nvarw >> nkp;
    _varw.resize (nvarw);
    _kp.resize (nkp);
    if (nvarw) {
	is >> _cpmap;
	if (nvarw != _cpmap.size())
	    XError::emit ("invalid font info");
	is.read (&_varw[0], nvarw * sizeof(decltype(_varw)::value_type));
	is.align (sizeof(decltype(_varw)::value_type));
	is.read (&_kp[0], nkp * sizeof(decltype(_varw)::value_type));
    }
}

void Info::write (bstro& os) const
{
    const uint16_t nvarw = _varw.size(), nkp = _kp.size();
    os << _w << _h << _b << _mw << _mh << _style << nvarw << nkp;
    if (nvarw) {
	os << _cpmap;
	os.write (&_varw[0], _varw.size() * sizeof(decltype(_varw)::value_type));
	os.align (sizeof(decltype(_varw)::value_type));
	os.write (&_kp[0], _kp.size() * sizeof(decltype(_varw)::value_type));
    }
}

void Info::write (bstrs& ss) const
{
    const uint16_t nvarw = _varw.size(), nkp = _kp.size();
    ss << _w << _h << _b << _mw << _mh << _style << nvarw << nkp;
    if (nvarw) {
	ss << _cpmap;
	ss.write (&_varw[0], _varw.size() * sizeof(decltype(_varw)::value_type));
	ss.align (sizeof(decltype(_varw)::value_type));
	ss.write (&_kp[0], _kp.size() * sizeof(decltype(_varw)::value_type));
    }
}

} // namespace Font

//----------------------------------------------------------------------

const char* TypeName (Type t) noexcept
{
    switch (t) {
	case BYTE:		return "BYTE"; break;
	case UNSIGNED_BYTE:	return "UNSIGNED_BYTE"; break;
	case SHORT:		return "SHORT"; break;
	case UNSIGNED_SHORT:	return "UNSIGNED_SHORT"; break;
	case INT:		return "INT"; break;
	case UNSIGNED_INT:	return "UNSIGNED_INT"; break;
	case FLOAT:		return "FLOAT"; break;
	case BYTES2:		return "BYTES2"; break;
	case BYTES3:		return "BYTES3"; break;
	case BYTES4:		return "BYTES4"; break;
	case DOUBLE:		return "DOUBLE"; break;
    }
    return "INVALID_TYPE";
}

const char* ShapeName (Shape s) noexcept
{
    switch (s) {
	case POINTS:		return "POINTS"; break;
	case LINES:		return "LINES"; break;
	case LINE_LOOP:		return "LINE_LOOP"; break;
	case LINE_STRIP:	return "LINE_STRIP"; break;
	case TRIANGLES:		return "TRIANGLES"; break;
	case TRIANGLE_STRIP:	return "TRIANGLE_STRIP"; break;
	case TRIANGLE_FAN:	return "TRIANGLE_FAN"; break;
	case LINES_ADJACENCY:	return "LINES_ADJACENCY"; break;
	case LINE_STRIP_ADJACENCY:	return "LINE_STRIP_ADJACENCY"; break;
	case TRIANGLES_ADJACENCY:	return "TRIANGLES_ADJACENCY"; break;
	case TRIANGLE_STRIP_ADJACENCY:	return "TRIANGLE_STRIP_ADJACENCY"; break;
	case PATCHES:		return "PATCHES"; break;
    }
    return "INVALID_SHAPE";
}

} // namespace G
