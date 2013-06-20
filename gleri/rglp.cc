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
     N(Open,(nnqqqyyyyyy))
     N(Close,)
     N(Draw,ay)
     N(LoadData,uqquuay)
     N(LoadFile,uqqh)
     N(LoadPakFile,uqqus)
     N(FreeResource,uq)
     N(BufferSubData,uqquay)
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
    return (CCmdBuf::CreateCmd (RGLObject, m, msz, sz, unwritten));
}

PRGL::goid_t PRGL::LoadFile (G::EResource dtype, const char* filename, G::EBufferHint hint)
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
