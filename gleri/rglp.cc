// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "rglp.h"
#include "mmfile.h"
#include <fcntl.h>

//----------------------------------------------------------------------

#define N(n,s)	#n "\0" #s "\0"
/*static*/ const char PRGL::_cmdNames[] =
     N(Auth,aysuuay)
     N(Open,(nnqqqyyyyyy)s)
     N(Close,)
     N(Draw,ay)
     N(Event,(unnuu))
     N(LoadData,uqquuay)
     N(LoadFile,uqqh)
     N(LoadPakFile,uqqus)
     N(FreeResource,uq)
     N(BufferSubData,uuay)
     N(TexParameter,qqi)
;
#undef N

/*static*/ inline const char* PRGL::LookupCmdName (ECmd cmd, size_type& sz) noexcept
{
    return (CCmdBuf::LookupCmdName((unsigned)cmd,sz,ArrayBlock(_cmdNames)-1));
}

/*static*/ PRGL::ECmd PRGL::LookupCmd (const char* name, size_type bleft) noexcept
{
    return (ECmd(CCmdBuf::LookupCmd(name,bleft,ArrayBlock(_cmdNames)-1)));
}

bstro PRGL::CreateCmd (ECmd cmd, size_type sz, size_type unwritten) noexcept
{
    size_type msz;
    const char* m = LookupCmdName (cmd, msz);
    return (CCmdBuf::CreateCmd (c_ObjectName, m, msz, sz, unwritten));
}

PRGL::goid_t PRGL::LoadFile (EResource dtype, const char* filename, uint16_t hint)
{
    goid_t id = GenId();
    CFile f (filename, O_RDONLY);
    uint32_t dsz = f.Size(), fsz = sizeof(int);
    if (!CanPassFd()) {
	fsz = Align(sizeof(dsz)+dsz,c_MsgAlignment);
	CmdU (ECmd::LoadData, fsz, id, dtype, hint, dsz, uint32_t(0));
    } else
	CmdU (ECmd::LoadFile, fsz, id, dtype, hint);
    SendFile (f, dsz);
    return (id);
}

//----------------------------------------------------------------------

namespace G {
const char* TypeName (Type t) noexcept
{
    switch (t) {
	case BYTE:		return ("BYTE"); break;
	case UNSIGNED_BYTE:	return ("UNSIGNED_BYTE"); break;
	case SHORT:		return ("SHORT"); break;
	case UNSIGNED_SHORT:	return ("UNSIGNED_SHORT"); break;
	case INT:		return ("INT"); break;
	case UNSIGNED_INT:	return ("UNSIGNED_INT"); break;
	case FLOAT:		return ("FLOAT"); break;
	case BYTES2:		return ("BYTES2"); break;
	case BYTES3:		return ("BYTES3"); break;
	case BYTES4:		return ("BYTES4"); break;
	case DOUBLE:		return ("DOUBLE"); break;
    }
    return ("INVALID_TYPE");
}

const char* ShapeName (Shape s) noexcept
{
    switch (s) {
	case POINTS:		return ("POINTS"); break;
	case LINES:		return ("LINES"); break;
	case LINE_LOOP:		return ("LINE_LOOP"); break;
	case LINE_STRIP:	return ("LINE_STRIP"); break;
	case TRIANGLES:		return ("TRIANGLES"); break;
	case TRIANGLE_STRIP:	return ("TRIANGLE_STRIP"); break;
	case TRIANGLE_FAN:	return ("TRIANGLE_FAN"); break;
	case LINES_ADJACENCY:	return ("LINES_ADJACENCY"); break;
	case LINE_STRIP_ADJACENCY:	return ("LINE_STRIP_ADJACENCY"); break;
	case TRIANGLES_ADJACENCY:	return ("TRIANGLES_ADJACENCY"); break;
	case TRIANGLE_STRIP_ADJACENCY:	return ("TRIANGLE_STRIP_ADJACENCY"); break;
	case PATCHES:		return ("PATCHES"); break;
    }
    return ("INVALID_SHAPE");
}
} // namespace G
