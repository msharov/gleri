// This file is part of the GLERI project
//
// Copyright (c) 2013 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "iconn.h"
#include "gwin.h"
#include ".o/data/data.h"

void CIConn::MapId (uint32_t cid, GLuint sid) noexcept
{
    DTRACE ("[fd %d] Map cid %x -> sid %x\n", Fd(), cid, sid);
    _cidmap.insert (SIdMap(cid,sid));
}

GLuint CIConn::LookupId (uint32_t cid) const noexcept
{
    auto fi = _cidmap.lower_bound (SIdMap(cid,0));
    if (fi != _cidmap.end() && fi->_cid == cid)
	return (fi->_sid);
    if (cid < G::default_Resources)
	return (_defres[cid]);
    return (CGObject::NoObject);
}

void CIConn::UnmapId (uint32_t cid) noexcept
{
    DTRACE ("[fd %d] Unmapping cid %x\n", Fd(), cid);
    erase_if (_cidmap, [cid](const SIdMap& i) { return (i._cid == cid); });
}

/*static*/ const CFont* CIConn::_deffont = nullptr;
/*static*/ GLuint CIConn::_defres [G::default_Resources] = {};

/*static*/ void CIConn::LoadDefaultResources (CGLWindow* w)
{
    DTRACE ("Loading shared resources\n");
    GLuint pak = w->LoadDatapak (ArrayBlock (File_resource));
    _defres[G::default_FlatShader] = w->LoadShader (pak, "sh/flat_v.glsl", "sh/flat_f.glsl");
    _defres[G::default_TextureShader] = w->LoadShader (pak, "sh/image_v.glsl", "sh/image_g.glsl", "sh/image_f.glsl");
    _defres[G::default_FontShader] = w->LoadShader (pak, "sh/font_v.glsl", "sh/image_g.glsl", "sh/font_f.glsl");
    _defres[G::default_Font] = w->LoadFont (pak, "ter-d18b.psf");
    w->FreeDatapak (pak);
    _deffont = w->Font(_defres[G::default_Font]);
}
