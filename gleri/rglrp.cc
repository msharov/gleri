// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "rglrp.h"

#define N(n,s)	#n "\0" s "\0"
const char PRGLR::_cmdNames[] =
     "\0"				// Dirty trick: object name is 4 bytes, but must be zero terminated, so there must be a null char here. This dirty trick continues below and is the reason for all the +1 and -1s.
     N(Restate,GLERI_WININFO_SIGNATURE)
     N(Expose,"")
     N(Event,GLERI_EVENT_SIGNATURE)
     N(SaveFB,"uuh")
     N(SaveFBData,"usuuay")
     N(ResInfo,"uqqay")
     N(ClipboardData,"uus")
;
#undef N

const char* PRGLR::LookupCmdName (ECmd cmd, size_type& sz) noexcept // static
{
    return CCmdBuf::LookupCmdName((unsigned)cmd,sz,_cmdNames+1,sizeof(_cmdNames)-2);
}

PRGLR::ECmd PRGLR::LookupCmd (const char* name, size_type bleft) noexcept // static
{
    return ECmd(CCmdBuf::LookupCmd(name+1,bleft,_cmdNames+1,sizeof(_cmdNames)-2));
}

bstro PRGLR::CreateCmd (ECmd cmd, size_type sz, size_type unwritten) noexcept
{
    size_type msz;
    auto m = LookupCmdName(cmd, msz);
    return CCmdBuf::CreateCmd (c_ObjectName, m-1, msz+1, sz, unwritten);
}

void PRGLR::SaveFB (goid_t id, const char* filename, CFile& f)
{
    uint32_t dsz = f.Size(), unwrsz = sizeof(int);
    if (!CanPassFd()) {
	bstrs ss;
	variadic_arg_size (ss, id, filename, dsz, uint32_t(0));
	unwrsz = Align(ss.size()+sizeof(dsz)+dsz,c_MsgAlignment)-ss.size();
	CmdU (ECmd::SaveFBData, unwrsz, id, filename, dsz, uint32_t(0));
    } else
	CmdU (ECmd::SaveFB, unwrsz, id, uint32_t(0));
    SendFile (f, dsz);
}
