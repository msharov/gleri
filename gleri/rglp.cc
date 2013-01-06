// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "rglp.h"

//----------------------------------------------------------------------

#define N(n,s)	#n "\0" #s "\0"
/*static*/ const char PRGL::_cmdNames[] =
     N(Open,hhu)
     N(Draw,ay)
     N(BufferData,uay)
     N(BufferSubData,uuay)
     N(FreeBuffer,u)
     N(LoadTexture,us)
     N(FreeTexture,u)
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
    return (CCmdBuf::CreateCmd (m, msz, sz));
}

