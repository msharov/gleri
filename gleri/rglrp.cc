// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "rglrp.h"

#define N(n,s)	#n "\0" #s "\0"
/*static*/ const char PRGLR::_cmdNames[] =
     N(Error,s)
     N(Restate,(qqqqbbbb))
     N(Draw,)
     N(Event,u)
;
#undef N

/*static*/ inline const char* PRGLR::LookupCmdName (ECmd cmd, size_type& sz) noexcept
{
    return (CCmdBuf::LookupCmdName((unsigned)cmd,sz,ArrayBlock(_cmdNames)-1));
}

/*static*/ PRGLR::ECmd PRGLR::LookupCmd (const char* name, size_type bleft) noexcept
{
    return (ECmd(CCmdBuf::LookupCmd(name,bleft,ArrayBlock(_cmdNames)-1)));
}

bstro PRGLR::CreateCmd (ECmd cmd, size_type sz) noexcept
{
    size_type msz;
    const char* m = LookupCmdName (cmd, msz);
    return (CCmdBuf::CreateCmd (m, msz, sz));
}
