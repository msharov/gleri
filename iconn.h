// This file is part of the GLERI project
//
// Copyright (c) 2013 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "config.h"
#include "gleri.h"
#include "goshad.h"
#include "gofont.h"

class CGLWindow;

class CIConn : public CCmdBuf {
    typedef PDraw<bstro>		draww_t;
    typedef G::goid_t			goid_t;
    typedef vector<unsigned char>	argv_t;
    typedef const argv_t&		rcargv_t;
public:
				CIConn (iid_t iid, int fd, bool fdpass);
				~CIConn (void) noexcept;
    void			VerifyFreeId (goid_t cid) const;
    inline bool			Authenticated (void) const	{ return (_authenticated); }
    inline void			SetAuthenticated (void)		{ _authenticated = true; }
    inline rcargv_t		Argv (void) const		{ return (_argv); }
    inline void			SetArgv (const SDataBlock& a)	{ _argv.assign ((const unsigned char*) a._p, (const unsigned char*) a._p+a._sz); }
    inline const string&	Hostname (void) const		{ return (_hostname); }
    inline void			SetHostname (const char* h)	{ _hostname.assign (h); }
    inline uint32_t		Pid (void) const		{ return (_pid); }
    const uint32_t*		PidPtr (void) const		{ return (&_pid); }
    inline void			SetPid (uint32_t pid)		{ _pid = pid; }
    inline uint32_t		Screen (void) const		{ return (_screen); }
    inline void			SetScreen (uint32_t screen)	{ _screen = screen; }
				// Shared resources
    void			LoadDefaultResources (CGLWindow* w);
    inline static bool		HaveDefaultResources (void)	{ return (_shwin); }
    const CShader&		DefaultShader (void) const	{ return (_shconn->LookupShader(G::default_FlatShader)); }
    const CShader&		GradientShader (void) const	{ return (_shconn->LookupShader(G::default_GradientShader)); }
    const CShader&		TextureShader (void) const	{ return (_shconn->LookupShader(G::default_TextureShader)); }
    const CShader&		FontShader (void) const		{ return (_shconn->LookupShader(G::default_FontShader)); }
    const CFont&		DefaultFont (void) const	{ return (_shconn->LookupFont(G::default_Font)); }
				// Resource loader by enum
    void			LoadResource (const CGLWindow* w, goid_t id, PRGL::EResource dtype, uint16_t hint, const GLubyte* d, GLuint dsz);
    void			LoadPakResource (const CGLWindow* w, goid_t id, PRGL::EResource dtype, uint16_t hint, const CDatapak& pak, const char* filename, GLuint flnsz);
    void			FreeResource (goid_t id, PRGL::EResource dtype);
				// Lookups for all resources
    const CDatapak&		LookupDatapak (goid_t id) const	{ return (LookupObject<CDatapak> (id, "no datapak %x")); }
    const CBuffer&		LookupBuffer (goid_t id) const	{ return (LookupObject<CBuffer> (id, "no buffer %x")); }
    const CShader&		LookupShader (goid_t id) const	{ return (LookupObject<CShader> (id, "no shader %x")); }
    const CTexture&		LookupTexture (goid_t id) const	{ return (LookupObject<CTexture> (id, "no texture %x")); }
    const CFramebuffer&		LookupFramebuffer (goid_t id) const { return (LookupObject<CFramebuffer> (id, "no framebuffer %x")); }
    const CFont&		LookupFont (goid_t id) const	{ return (LookupObject<CFont> (id, "no font %x")); }
private:
    inline const CDatapak&	LoadDatapak (const CGLWindow* w, goid_t cid, const GLubyte* p, GLuint psz);
    inline void			LoadBuffer (const CGLWindow* w, goid_t cid, const void* data, GLuint dsz, G::BufferHint mode, G::BufferType btype);
    inline void			LoadShader (const CGLWindow* w, goid_t cid, const char* v, const char* tc, const char* te, const char* g, const char* f);
    void			LoadShader (const CGLWindow* w, goid_t cid, const CDatapak& pak, const char* v, const char* tc, const char* te, const char* g, const char* f);
    inline void			LoadShader (const CGLWindow* w, goid_t cid, const CDatapak& pak, const char* v, const char* g, const char* f);
    inline void			LoadShader (const CGLWindow* w, goid_t cid, const CDatapak& pak, const char* v, const char* f);
    inline void			LoadTexture (const CGLWindow* w, goid_t cid, const GLubyte* d, GLuint dsz, G::Pixel::Fmt storeas, G::TextureType ttype);
    inline void			LoadFramebuffer (const CGLWindow* w, goid_t cid, const GLubyte* d, GLuint dsz);
    inline void			LoadFont (const CGLWindow* w, goid_t cid, const GLubyte* p, GLuint psz);
				// Misc
    void			AddObject (CGObject* o);
    const CGObject*		FindObject (goid_t cid) const noexcept;
    static void			ShaderUnpack (const GLubyte* s, GLuint ssz, const char* shs[5]) noexcept;
    template <typename O>
    const O&			LookupObject (goid_t cid, const char* errstr = "no object %x") const {
				    const O* po = dynamic_cast<const O*>(FindObject (cid));
				    if (!po) throw XError (errstr, cid);
				    return (*po);
				}
private:
    bool			_authenticated	= false;
    vector<CGObject*>		_obj;
    argv_t			_argv;
    string			_hostname;
    uint32_t			_pid;
    uint32_t			_screen;
    static const CGLWindow*	_shwin;
    static const CIConn*	_shconn;
};
