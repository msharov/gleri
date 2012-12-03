#include <gleri.h>
#include <gleri/app.h>
#include <gleri/rglp.h>
#include <sys/socket.h>
#include <unistd.h>

class CWindow {
public:
    typedef CCmd::iid_t	wid_t;
public:
    inline explicit	CWindow (wid_t wid)	: _prgl(wid) { _prgl.Open (640,480,0x33); OnInit(); }
    void		OnExpose (void);
    void		OnInit (void);
    inline void		OnResize (uint16_t w, uint16_t h);
    inline void		OnEvent (uint32_t key);
    ONDRAWDECL		OnDraw (Drw& drw) const;
    inline void		WriteCmds (int fd)	{ _prgl.WriteToFd (fd); }
private:
    PRGL		_prgl;
    uint32_t		_vbuf;
    uint32_t		_walk;
};

//----------------------------------------------------------------------
namespace {

static const uint16_t _vdata1[] = {
    0,0, 0,479, 639,479, 639,0,
    50,50, 50,300, 150,300, 150,50,
    100,100, 200,200, 300,200, 300,300,
    250,250, 250,400, 350,250, 500,450,
    250,50, 300,100, 300,50, 350,75,
};
enum {
    vb_WindowBorderOffset,
    vb_WindowBorderSize = 4,	// In vertices
    vb_PurpleQuadOffset = vb_WindowBorderOffset+vb_WindowBorderSize,
    vb_PurpleQuadSize = 4,
    vb_BrokenLineOffset = vb_PurpleQuadOffset+vb_PurpleQuadSize,
    vb_BrokenLineSize = 4,
    vb_TransparentStripOffset = vb_BrokenLineOffset+vb_BrokenLineSize,
    vb_TransparentStripSize = 4,
    vb_SkewQuadOffset = vb_TransparentStripOffset+vb_TransparentStripSize,
    vb_SkewQuadSize = 4,
    vb_nVertices
};

} // namespace
//----------------------------------------------------------------------

void CWindow::OnInit (void)
{
    _prgl.BufferData (_vbuf = _prgl.CreateBuffer(), _vdata1, sizeof(_vdata1));
    _prgl.LoadTexture (_walk = _prgl.CreateTexture(), "walk.png");
}

ONDRAWIMPL CWindow::OnDraw (Drw& drw) const
{
    drw.Clear (G::RGB(0,0,64));

    drw.DefaultShader();
    drw.VertexPointer (_vbuf);

    drw.Color (0,255,255);
    drw.LineLoop (vb_WindowBorderOffset, vb_WindowBorderSize);
    drw.Color (128,90,150);
    drw.TriangleFan (vb_PurpleQuadOffset, vb_PurpleQuadSize);
    drw.Color (255,255,255);
    drw.LineStrip (vb_BrokenLineOffset, vb_BrokenLineSize);

    drw.Sprite (200, 75, _walk);

    drw.DefaultShader();
    drw.VertexPointer (_vbuf);

    drw.Color (G::ARGB(0xc0804040));
    drw.TriangleStrip (vb_TransparentStripOffset, vb_TransparentStripSize);
    drw.Color (128,170,170);
    drw.TriangleStrip (vb_SkewQuadOffset, vb_SkewQuadSize);

    drw.Color (0,240,255,128);
    drw.Text (300, 250, "Hello world from OpenGL!");

    drw.Color (255,255,255);
    drw.Text (300, 420, "A quick brown fox jumps over the lazy dog");
}

void CWindow::OnExpose (void)
{
    printf ("Drawing test window\n");
    Draw();
}

void CWindow::OnResize (uint16_t w, uint16_t h)
{
    printf ("Test window OnResize %hux%hu\n", w, h);
    const uint16_t sw = w-1, sh = h-1;
    const uint16_t _vdata1[] = { sh, sw,sh, sw };
    _prgl.BufferSubData (_vbuf, _vdata1, sizeof(_vdata1), 3*sizeof(short));
}

void CWindow::Draw (void)
{
    PDraw<bstrs> drws;
    OnDraw (drws);
    auto drww = _prgl.Draw (drws.size());
    OnDraw (drww);
}

class CGLTest : public CApp {
public:
    inline			~CGLTest (void) noexcept	{ close (_srvsock); }
    static inline CGLTest&	Instance (void)	{ static CGLTest s_App; return (s_App); }
    void			Init (argc_t argc, argv_t argv);
private:
    inline			CGLTest (void);
    int				LaunchServer (void) noexcept;
    virtual void		OnFd (int fd);
    virtual void		OnFdError (int fd);
    static void			OnSignal (int sig) noexcept;
private:
    vector<CWindow>		_wins;
    CCmdBuf			_srvbuf;
    int				_srvsock;
    pid_t			_srvpid;
};

inline void CWindow::OnEvent (uint32_t key)
{
    if (key == 24 || key == 9) {
	printf ("Event received. Quitting\n");
	CGLTest::Instance().Quit();
    }
}

//----------------------------------------------------------------------

inline CGLTest::CGLTest (void)
: CApp()
,_wins()
,_srvbuf()
,_srvsock(-1)
,_srvpid(0)
{
}

void CGLTest::Init (argc_t argc, argv_t argv)
{
    CApp::Init (argc, argv);
    _srvpid = LaunchServer();
    WatchFd (_srvsock);
    _wins.emplace_back (44);
    _wins.back().WriteCmds (_srvsock);
}

int CGLTest::LaunchServer (void) noexcept
{
    static const char* const c_srvcmd[] = { GLERIS_NAME, "-s", nullptr };

    int sock[2];
    if (0 > socketpair (PF_LOCAL, SOCK_STREAM| SOCK_NONBLOCK, 0, sock)) {
	perror ("socketpair");
	exit (EXIT_FAILURE);
    }

    pid_t cpid = fork();
    if (cpid < 0)
	perror ("fork");
    else if (cpid > 0) {
	_srvsock = sock[0];
	close (sock[1]);
	return (cpid);
    } else {
	close (sock[0]);
	dup2 (sock[1], 0);
	if (0 > execve (c_srvcmd[0], const_cast<char* const*>(c_srvcmd), environ))
	    perror ("execve");
    }

    exit (EXIT_FAILURE);
}

void CGLTest::OnFd (int fd)
{
    _srvbuf.ReadFromFd (fd);
    PRGLR::Parse (_wins[0], _srvbuf);
    _wins[0].WriteCmds (fd);
}

void CGLTest::OnFdError (int fd)
{
    CApp::OnFdError (fd);
    printf ("gleris connection terminated\n");
    Quit();
}

GLERI_APP (CGLTest)
