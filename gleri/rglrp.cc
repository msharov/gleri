// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "rglrp.h"

#define N(n,s)	#n "\0" #s "\0"
/*static*/ const char PRGLR::_cmdNames[] =
     "\0"	// Dirty trick: object name is 4 bytes, but must be zero terminated
     N(GLError,s)
     N(Restate,(qqqqyyyy))
     N(Expose,)
     N(Event,(uqquqq))
;
#undef N

/*static*/ inline const char* PRGLR::LookupCmdName (ECmd cmd, size_type& sz) noexcept
{
    return (CCmdBuf::LookupCmdName((unsigned)cmd,sz,_cmdNames+1,sizeof(_cmdNames)-2));
}

/*static*/ PRGLR::ECmd PRGLR::LookupCmd (const char* name, size_type bleft) noexcept
{
    return (ECmd(CCmdBuf::LookupCmd(name+1,bleft,_cmdNames+1,sizeof(_cmdNames)-2)));
}

bstro PRGLR::CreateCmd (ECmd cmd, size_type sz) noexcept
{
    size_type msz;
    const char* m = LookupCmdName(cmd, msz);
    return (CCmdBuf::CreateCmd (RGLRObject, m-1, msz+1, sz));
}
