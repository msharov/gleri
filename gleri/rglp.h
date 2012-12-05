#pragma once
#include "cmd.h"
#include "drawp.h"

class PRGL : public CCmdBuf {
public:
    typedef PDraw<bstro>	draww_t;
private:
    enum class ECmd : uint32_t {
	Open,
	Draw,
	BufferData,
	BufferSubData,
	FreeBuffer,
	LoadTexture,
	FreeTexture,
	NCmds,
    };
public:
    inline			PRGL (int fd, iid_t iid)	: CCmdBuf(fd,iid),_nextid(0) {}
				// Command writing
    inline void			Open (uint16_t w, uint16_t h, uint32_t glver = 0x33)	{ Cmd(ECmd::Open,w,h,glver); }
    inline draww_t		Draw (size_type sz)		{ return (draww_t(CreateCmd (ECmd::Draw,sz))); }
    inline uint32_t		CreateBuffer (void)		{ return (GenId()); }
    inline void			BufferData (uint32_t id, const void* data, uint32_t dsz, G::EBufferHint hint = G::STATIC_DRAW, G::EBufferType btype = G::ARRAY_BUFFER);
    inline void			BufferSubData (uint32_t id, const void* data, uint32_t dsz, uint16_t offset = 0, G::EBufferType btype = G::ARRAY_BUFFER);
    inline void			FreeBuffer (uint32_t id)	{ Cmd(ECmd::FreeBuffer,id); }
    inline uint32_t		CreateTexture (void)		{ return (GenId()); }
    inline void			LoadTexture (uint32_t id, const char* f)	{ Cmd(ECmd::LoadTexture,id,f); }
    inline void			FreeTexture (uint32_t id)	{ Cmd(ECmd::FreeTexture,id); }
				// Buffer reading for serialization
    inline bstri		BeginRead (void)		{ return (CCmdBuf::BeginRead()); }
    inline void			EndRead (const bstri& is)	{ CCmdBuf::EndRead(is); }
    template <typename F>
    static inline void		Parse (F& f, CCmdBuf& cmdbuf);
private:
    template <typename... Arg>
    inline void			Cmd (ECmd cmd, const Arg&... args);
    bstro			CreateCmd (ECmd cmd, size_type sz) noexcept;
    inline uint32_t		GenId (void)			{ return (++_nextid); }
    static inline const char*	LookupCmdName (ECmd cmd, size_type& sz) noexcept;
    static ECmd			LookupCmd (const char* name, size_type bleft) noexcept;
private:
    uint32_t			_nextid;
    static const char		_cmdNames[];
};

template <typename... Arg>
inline void PRGL::Cmd (ECmd cmd, const Arg&... args)
{
    bstrs ss;
    variadic_arg_size (ss, args...);
    bstro os = CreateCmd (cmd, ss.size());
    variadic_arg_write (os, args...);
}

inline void PRGL::BufferData (uint32_t id, const void* data, uint32_t dsz, G::EBufferHint hint, G::EBufferType btype)
    { Cmd (ECmd::BufferData, id, btype, hint, SDataBlock (data, dsz)); }
inline void PRGL::BufferSubData (uint32_t id, const void* data, uint32_t dsz, uint16_t offset, G::EBufferType btype)
    { Cmd (ECmd::BufferSubData, id, btype, offset, SDataBlock (data, dsz)); }

template <typename F>
/*static*/ inline void PRGL::Parse (F& f, CCmdBuf& cmdbuf)
{
    size_type sz; iid_t iid; uint8_t hsz; uint32_t objn;	// All commands start with these
    const size_type chsz = sizeof(sz)+sizeof(iid)+sizeof(hsz)+sizeof(objn);

    bstri is = cmdbuf.BeginRead();

    while (is.remaining() > chsz) {	// While have commands
	auto ihdr = is.ipos();		// Save header start for return
	is >> sz >> iid >> hsz >> objn;
	if (is.remaining() < (hsz-=chsz)+sz) {
	    is.iseek (ihdr);		// Restart at header
	    break;
	}
	auto clir = f.ClientRecord(cmdbuf.Fd(), iid);
	if (objn != RGLObject)	// Not for me
	    Error();

	bstri cmdis (is.ipos()+hsz, sz);	// Command data stream
	const char* cmdname = (const char*) is.ipos();
	is.skip (hsz+sz);			// Skip to next command

	ECmd cmd = LookupCmd (cmdname, hsz);
	if (!clir ^ (cmd == ECmd::Open))
	    Error();

	switch (cmd) {
	    case ECmd::Open: {
		uint16_t w,h; uint32_t glver;
		cmdis >> w >> h >> glver;
		f.CreateClient (cmdbuf.Fd(), iid, w, h, glver);
		} break;
	    case ECmd::Draw:
		f.ClientDraw (*clir,cmdis);
		break;
	    case ECmd::BufferData: {
		uint32_t id, dsz; uint16_t btype, hint;
		if (cmdis.remaining() < 12) Error();
		cmdis >> id >> btype >> hint >> dsz;
		if (cmdis.remaining() < dsz) Error();
		uint32_t sid = clir->LookupId (id);
		if (sid == UINT32_MAX) {
		    sid = f.CreateBuffer();
		    clir->MapId (id, sid);
		}
		f.BufferData (sid, cmdis.ipos(), dsz, hint, btype);
		} break;
	    case ECmd::BufferSubData: {
		uint32_t id, dsz; uint16_t offset, btype;
		if (cmdis.remaining() < 12) Error();
		cmdis >> id >> btype >> offset >> dsz;
		if (cmdis.remaining() < dsz) Error();
		f.BufferSubData (clir->LookupId(id), cmdis.ipos(), dsz, offset, btype);
		} break;
	    case ECmd::FreeBuffer: {
		uint32_t id;
		if (cmdis.remaining() < 4) Error();
		cmdis >> id;
		f.FreeBuffer(clir->LookupId(id));
		clir->UnmapId(id);
		} break;
	    case ECmd::LoadTexture: {
		uint32_t id;
		const char* tfn;
		if (cmdis.remaining() < 8) Error();
		cmdis >> id >> tfn;
		if (!tfn) Error();
		clir->MapId(id,f.LoadTexture(tfn));
		} break;
	    case ECmd::FreeTexture: {
		uint32_t id;
		if (cmdis.remaining() < 4) Error();
		cmdis >> id;
		f.FreeTexture(clir->LookupId(id));
		clir->UnmapId(id);
		} break;
	    default:
		Error();
	}
    }
    cmdbuf.EndRead(is);
}
