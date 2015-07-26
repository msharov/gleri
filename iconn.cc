// This file is part of the GLERI project
//
// Copyright (c) 2013 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "iconn.h"
#include "gwin.h"
#include ".o/data/data.h"

/*static*/ const CGLWindow* CIConn::_shwin = nullptr;;
/*static*/ const CIConn* CIConn::_shconn = nullptr;;

CIConn::CIConn (iid_t iid, int fd, bool fdpass)
: CCmdBuf(iid,fd,fdpass)
,_obj()
,_argv()
,_hostname()
,_pid(0)
{
    if (!_shconn)
	_shconn = this;
}

CIConn::~CIConn (void) noexcept
{
    for (auto o : _obj) {
	DTRACE ("Deleting object cid %x, sid %x\n", o->CId(), o->Id());
	delete o;
    }
    _obj.clear();
    Outfile().ForceClose();
}

void CIConn::VerifyFreeId (goid_t cid) const
{
    if (FindObject (cid))
	throw XError ("object 0x%x already exists\n", cid);
}

const CGObject* CIConn::FindObject (goid_t cid) const noexcept
{
    if (cid < G::default_ResourceMaxId && this != _shconn && HaveDefaultResources())
	return _shconn->FindObject (cid);
    auto io = lower_bound (_obj.begin(), _obj.end(), cid, [](const CGObject* o, goid_t id) { return o->CId() < id; });
    return (io != _obj.end() && (*io)->CId() == cid) ? *io : nullptr;
}

void CIConn::AddObject (unique_ptr<CGObject>&& o)
{
    if (o == nullptr || o->CId() == G::GoidNull || o->Id() == CGObject::NoObject)
	throw XError ("failed create resource object %x", o->CId());
    auto io = lower_bound (_obj.begin(), _obj.end(), o.get(), [](const CGObject* o1, const CGObject* o2) { return *o1 < *o2; });
    DTRACE ("Inserting object cid %x, sid %x\n", o->CId(), o->Id());
    _obj.insert (io, o.release());
}

//----------------------------------------------------------------------

void CIConn::LoadDefaultResources (CGLWindow* w)
{
    DTRACE ("Loading shared resources\n");
    AddObject (unique_ptr<CGObject>(new CFramebuffer (w->ContextId(), G::default_Framebuffer, 0)));
    const auto& pak = LoadDatapak (w, G::default_ResourcePak, ArrayBlock (File_resource));
    LoadShader (w, G::default_FlatShader, pak, "sh/flat_v.glsl", "sh/flat_f.glsl");
    LoadShader (w, G::default_GradientShader, pak, "sh/grad_v.glsl", "sh/grad_f.glsl");
    LoadShader (w, G::default_TextureShader, pak, "sh/image_v.glsl", "sh/image_g.glsl", "sh/image_f.glsl");
    LoadShader (w, G::default_FontShader, pak, "sh/font_v.glsl", "sh/image_g.glsl", "sh/font_f.glsl");
    LoadPakResource (w, G::default_Font, PRGL::EResource::FONT, 0, pak, "ter-d18b.psf", strlen("ter-d18b.psf"));
    FreeResource (G::default_ResourcePak, PRGL::EResource::DATAPAK);
    _shwin = w;
    #ifndef NDEBUG
	w->CheckForErrors();
    #endif
}

//----------------------------------------------------------------------
// Resource loader by enum

/*static*/ void CIConn::ShaderUnpack (const GLubyte* s, GLuint ssz, const char* shs[5]) noexcept
{
    bstri shis (s, ssz);
    for (auto i = 0; i < 5; ++i) {
	shs[i] = shis.read_strz();
	if (!*shs[i])
	    shs[i] = nullptr;
    }
}

void CIConn::LoadResource (CGLWindow* w, goid_t id, PRGL::EResource dtype, uint16_t hint, const GLubyte* d, GLuint dsz)
{
    if (dtype == PRGL::EResource::DATAPAK)
	LoadDatapak (w, id, d, dsz);
    else if (dtype >= PRGL::EResource::_BUFFER_FIRST && dtype <= PRGL::EResource::_BUFFER_LAST)
	LoadBuffer (w, id, d, dsz, G::BufferHint(hint), PRGL::BufferTypeFromResource(dtype));
    else if (dtype >= PRGL::EResource::_TEXTURE_FIRST && dtype <= PRGL::EResource::_TEXTURE_LAST)
	LoadTexture (w, id, d, dsz, G::Pixel::Fmt(hint), PRGL::TextureTypeFromResource(dtype));
    else if (dtype == PRGL::EResource::FRAMEBUFFER)
	LoadFramebuffer (w, id, d, dsz);
    else if (dtype == PRGL::EResource::FONT)
	LoadFont (w, id, d, dsz, hint);
    else if (dtype == PRGL::EResource::SHADER) {
	const char* shs[5];
	ShaderUnpack (d, dsz, shs);
	LoadShader (w, id, shs[0],shs[1],shs[2],shs[3],shs[4]);
    } else
	throw XError ("invalid resource type %u", dtype);
}

void CIConn::LoadPakResource (CGLWindow* w, goid_t id, PRGL::EResource dtype, uint16_t hint, const CDatapak& pak, const char* filename, GLuint flnsz)
{
    if (dtype == PRGL::EResource::SHADER) {
	const char* shs[5];
	ShaderUnpack ((const uint8_t*) filename, flnsz, shs);
	LoadShader (w, id, pak, shs[0],shs[1],shs[2],shs[3],shs[4]);
    } else {
	GLuint fsz;
	auto pf = pak.File (filename, fsz);
	if (!pf) throw XError ("%s is not in the datapak", filename);
	LoadResource (w, id, dtype, hint, pf, fsz);
    }
}

void CIConn::FreeResource (goid_t cid, PRGL::EResource)
{
    DTRACE ("[fd %d] FreeResource %x\n", Fd(), cid);
    auto io = lower_bound (_obj.begin(), _obj.end(), cid, [](const CGObject* o, goid_t id) { return o->CId() < id; });
    if (io != _obj.end() && (*io)->CId() == cid) {
	DTRACE ("[fd %d] Deleting object %x, sid %x\n", Fd(), (*io)->CId(), (*io)->Id());
	delete *io;
	_obj.erase (io);
    }
}

void CIConn::FreeResources (const CGLWindow* w)
{
    DTRACE ("[%x] Freeing all resources in context %x\n", w->IId(), w->ContextId());
    for (auto r = _obj.begin(); r < _obj.end(); ++r) {
	if ((*r)->Context() == w->ContextId()) {
	    DTRACE ("[%x] Deleting object %x, sid %x\n", w->IId(), (*r)->CId(), (*r)->Id());
	    delete (*r);
	    --(r = _obj.erase(r));
	}
    }
}

//----------------------------------------------------------------------

inline const CDatapak& CIConn::LoadDatapak (CGLWindow* w, goid_t cid, const GLubyte* pi, GLuint isz)
{
    DTRACE ("[%x] LoadDatapak %x from %u bytes\n", w->IId(), cid, isz);
    auto osz = 0u;
    auto po = CDatapak::DecompressBlock (pi, isz, osz);
    if (!po) XError::emit ("failed to decompress datapak");
    auto pdpk = new CDatapak (w->ContextId(), cid, move(po), osz);
    AddObject (unique_ptr<CGObject>(pdpk));
    return *pdpk;
}

inline void CIConn::LoadBuffer (CGLWindow* w, goid_t cid, const void* data, GLuint dsz, G::BufferHint hint, G::BufferType btype)
{
    DTRACE ("[%x] CreateBuffer %x type %x, hint %x, %u bytes\n", w->IId(), cid, btype, hint, dsz);
    AddObject (unique_ptr<CGObject>(new CBuffer (w->ContextId(), cid, data, dsz, hint, btype)));
}

inline void CIConn::LoadShader (CGLWindow* w, goid_t cid, const char* v, const char* tc, const char* te, const char* g, const char* f)
{
    DTRACE ("[%x] LoadShader %x\n", w->IId(), cid);
    AddObject (unique_ptr<CGObject>(new CShader (w->ContextId(), cid, CShader::Sources(v,tc,te,g,f))));
}

void CIConn::LoadShader (CGLWindow* w, goid_t cid, const CDatapak& pak, const char* v, const char* tc, const char* te, const char* g, const char* f)
{
    DTRACE ("[%x] LoadShader %x from pak %x: %s,%s,%s,%s,%s\n", w->IId(), cid, pak.Id(),v,tc,te,g,f);
    AddObject (unique_ptr<CGObject>(new CShader (w->ContextId(), cid, CShader::Sources(pak,v,tc,te,g,f))));
}
inline void CIConn::LoadShader (CGLWindow* w, goid_t cid, const CDatapak& pak, const char* v, const char* g, const char* f)
    { LoadShader(w,cid,pak,v,nullptr,nullptr,g,f); }
inline void CIConn::LoadShader (CGLWindow* w, goid_t cid, const CDatapak& pak, const char* v, const char* f)
    { LoadShader(w,cid,pak,v,nullptr,nullptr,nullptr,f); }

inline void CIConn::LoadTexture (CGLWindow* w, goid_t cid, const GLubyte* d, GLuint dsz, G::Pixel::Fmt storeas, G::TextureType ttype)
{
    DTRACE ("[%x] LoadTexture %x type %u from %u bytes\n", w->IId(), cid, ttype, dsz);
    auto t = new CTexture (w->ContextId(), cid, d, dsz, storeas, ttype, w->TexParams());
    AddObject (unique_ptr<CGObject>(t));
    w->ResourceInfo (cid, uint16_t(PRGL::ResourceFromTextureType(ttype)), t->Info());
}

inline void CIConn::LoadFramebuffer (CGLWindow* w, goid_t cid, const GLubyte* d, GLuint dsz)
{
    DTRACE ("[%x] LoadFramebuffer %x from %u bytes\n", w->IId(), cid, dsz);
    AddObject (unique_ptr<CGObject>(new CFramebuffer (w->ContextId(), cid, d, dsz, *this)));
}

inline void CIConn::LoadFont (CGLWindow* w, goid_t cid, const GLubyte* p, GLuint psz, uint8_t fontSize)
{
    DTRACE ("[%x] LoadFont %x from %u bytes, varsize %hhu\n", w->IId(), cid, psz, fontSize);
    auto f = new CFont (w->ContextId(), cid, p, psz, fontSize);
    AddObject (unique_ptr<CGObject>(f));
    if (cid > G::default_ResourceMaxId)
	w->ResourceInfo (cid, uint16_t(PRGL::EResource::FONT), f->Info());
}
