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
     N(Draw,uay)
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
    return CCmdBuf::LookupCmdName((unsigned)cmd,sz,ArrayBlock(_cmdNames)-1);
}

/*static*/ PRGL::ECmd PRGL::LookupCmd (const char* name, size_type bleft) noexcept
{
    return ECmd(CCmdBuf::LookupCmd(name,bleft,ArrayBlock(_cmdNames)-1));
}

bstro PRGL::CreateCmd (ECmd cmd, size_type sz, size_type unwritten) noexcept
{
    size_type msz;
    const char* m = LookupCmdName (cmd, msz);
    return CCmdBuf::CreateCmd (c_ObjectName, m, msz, sz, unwritten);
}

PRGL::goid_t PRGL::LoadFile (EResource dtype, const char* filename, uint16_t hint)
{
    goid_t id = GenId();
    CFile f (filename, O_RDONLY);
    uint32_t dsz = f.Size(), unwrsz = sizeof(int);
    if (!CanPassFd()) {
	unwrsz = Align(sizeof(dsz)+dsz,c_MsgAlignment);
	CmdU (ECmd::LoadData, unwrsz, id, dtype, hint, dsz, uint32_t(0));
    } else
	CmdU (ECmd::LoadFile, unwrsz, id, dtype, hint);
    SendFile (f, dsz);
    return id;
}
