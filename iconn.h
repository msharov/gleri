// This file is part of the GLERI project
//
// Copyright (c) 2013 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "config.h"
#include "gleri.h"
#include "gofont.h"

class CGLWindow;

class CIConn : public CCmdBuf {
    typedef PDraw<bstro>	draww_t;
    typedef draww_t::goid_t	goid_t;
    enum : goid_t { GoidNull = numeric_limits<goid_t>::max() };
    struct SIdMap {
	union {
	    struct {
		GLuint		_sid;
		goid_t		_cid;
	    };
	    uint64_t	_key;
	};
	inline		SIdMap (goid_t c, GLuint s)		:_sid(s),_cid(c) {}
	inline bool	operator< (const SIdMap& v) const	{ return (_key < v._key); }
	inline bool	operator== (const SIdMap& v) const	{ return (_key == v._key); }
    };
    typedef vector<unsigned char>	argv_t;
    typedef const argv_t&		rcargv_t;
public:
    inline		CIConn (iid_t iid, int fd, bool fdpass)	: CCmdBuf(iid,fd,fdpass),_cidmap(),_argv(),_hostname(),_pid(0) {}
    void		MapId (goid_t cid, GLuint sid) noexcept;
    GLuint		LookupId (goid_t cid) const noexcept;
    void		UnmapId (goid_t cid) noexcept;
    inline bool		Authenticated (void) const	{ return (_authenticated); }
    inline void		SetAuthenticated (void)		{ _authenticated = true; }
    inline rcargv_t	Argv (void) const		{ return (_argv); }
    inline void		SetArgv (const SDataBlock& a)	{ _argv.assign ((const unsigned char*) a._p, (const unsigned char*) a._p+a._sz); }
    inline const string& Hostname (void) const		{ return (_hostname); }
    inline void		SetHostname (const char* h)	{ _hostname.assign (h); }
    inline uint32_t	Pid (void) const		{ return (_pid); }
    const uint32_t*	PidPtr (void) const		{ return (&_pid); }
    inline void		SetPid (uint32_t pid)		{ _pid = pid; }
			// Shared resources
    static void		LoadDefaultResources (CGLWindow* w);
    inline GLuint	DefaultShader (void) const	{ return (_defres [G::default_FlatShader]); }
    inline GLuint	TextureShader (void) const	{ return (_defres [G::default_TextureShader]); }
    inline GLuint	FontShader (void) const		{ return (_defres [G::default_FontShader]); }
    inline GLuint	DefaultFontId (void) const	{ return (_defres [G::default_Font]); }
    inline const CFont*	DefaultFont (void) const	{ return (_deffont); }
private:
    bool		_authenticated	= false;
    set<SIdMap>		_cidmap;
    argv_t		_argv;
    string		_hostname;
    uint32_t		_pid;
    static GLuint	_defres [G::default_Resources];
    static const CFont*	_deffont;
};
