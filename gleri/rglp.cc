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
     N(Open,(qqqqyyyy))
     N(Close,)
     N(Draw,ay)
     N(LoadResource,uqqay)
     N(LoadFile,uqqh)
     N(FreeResource,uq)
     N(BufferSubData,uqqay)
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

bstro PRGL::CreateCmd (ECmd cmd, size_type sz) noexcept
{
    size_type msz;
    const char* m = LookupCmdName (cmd, msz);
    return (CCmdBuf::CreateCmd (RGLObject, m, msz, sz));
}

uint32_t PRGL::LoadTexture (const char* filename)
{
    uint32_t id = GenId();
    CFile f (filename, O_RDONLY);
    Cmd (ECmd::LoadFile, id, G::EResource::TEXTURE, G::STATIC_DRAW, f.Fd());
    SendFile (f);
    return (id);
}

