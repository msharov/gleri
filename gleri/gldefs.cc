// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gldefs.h"

namespace G {

//----------------------------------------------------------------------

namespace Font {

dim_t Info::Width (const char* s) const
{
    if (_varw.empty())
	return Width()*strlen(s);
    dim_t sw = 0;
    for (; *s; ++s)
	sw += Width (*s);
    return sw;
}

void Info::read (bstri& is)
{
    uint16_t nvarw;
    is >> _w >> _h >> _ascent >> nvarw;
    _varw.resize(nvarw);
    is.read (&_varw[0], nvarw*sizeof(dim_t));
}

void Info::write (bstro& os) const
{
    os << _w << _h << _ascent << uint16_t(_varw.size());
    os.write (&_varw[0], _varw.size()*sizeof(dim_t));
}

void Info::write (bstrs& ss) const
{
    ss << _w << _h << _ascent << uint16_t(_varw.size());
    ss.write (&_varw[0], _varw.size()*sizeof(dim_t));
}

}

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
