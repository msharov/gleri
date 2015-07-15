// This is an image viewer, a tutorial illustrating loading of resources
// and more extensive use of the drawlist functionality.

#include "../../gleri.h"
#include <sys/stat.h>
#include <dirent.h>

class CImageViewer : public CWindow {
    enum {
	c_ScrollStep = 16,				// Scrolling step in image view
	c_ThumbWidth = 128,				// Thumbnail width
	c_ThumbHeight = c_ThumbWidth,			// Thumbnail height
	c_EntryWidth = c_ThumbWidth*7/4,		// Full file entry width
	c_EntryHeight = c_EntryWidth,
	c_ThumbX = (c_EntryWidth-c_ThumbWidth)/2,	// Offset of thumbnail in entry box
	c_ThumbY = c_ThumbWidth/4,
	c_CacheNEntriesX = 8,				// Number of thumbs in cache row
	c_CacheNEntriesY = c_CacheNEntriesX,		// ... column
	c_CacheNEntries = c_CacheNEntriesX*c_CacheNEntriesY,
	c_CacheWidth = c_CacheNEntriesX*c_ThumbWidth,	// Cache texture width in pixels
	c_CacheHeight = c_CacheNEntriesY*c_ThumbHeight,	// Cache texture width in pixels
    };
    enum : color_t {					// Color scheme
	color_ImageViewBackground	= RGB(0,0,0),
	color_FolderViewBackground	= RGB(0,0,0),
	color_FolderViewSelection	= RGB(32,32,32),
	color_FolderViewText		= RGB(128,128,128),
	color_ThumbBackground		= RGBA(0,0,0,0)
    };
    // It is efficient to combine all vertices into a single vertex buffer object,
    // but keeping track of the separate objects in it can be tedious. The VRENUM
    // macro defines v_NameOffset and v_NameSize (in vertices) that can be passed
    // to primitive drawing commands. The image viewer needs three objects:
    enum {
	VRENUM (EntrySquare, 4),	// The area of each file thumbnail and name. Also used as a selector.
	VRENUM (FolderIcon, 7),		// The folder icon, as a line loop, drawn for folders in the selector view
	VRENUM (FileIcon, 8)		// The file icon, drawn for files without a thumbnail
    };
public:
    explicit		CImageViewer (iid_t wid, const char* filename);
    virtual void	OnInit (void) override;
    ONDRAWDECL		OnDraw (Drw& drw) const;
    virtual void	OnKey (key_t key) override;
    virtual void	OnTextureInfo (goid_t, const G::Texture::Header&) override;
    virtual void	OnResize (dim_t w, dim_t h) override;
private:
    // There are two possible views, shown in the same window
    enum EView {
	ImageView,	// Shows the image
	FolderView	// Shows the folder thumbnail view
    };
    //{{{ CFolderEntry - a folder entry record
    class CFolderEntry {
    public:
	enum EType : uint8_t { Folder, Image };
    public:
	inline		CFolderEntry (const char* name, uint32_t size, bool isFile = true)
				:_size(size),_w(0),_h(0),_type(isFile ? Image : Folder),_thumb(_type) { _name = name; }
	inline bool	operator< (const char* name) const	{ return strcmp (_name.c_str(), name) < 0; }
	inline bool	operator< (const CFolderEntry& e) const	{ return _name < e._name; }
	inline bool	operator== (const CFolderEntry& e)const	{ return _name == e._name; }
	inline const char*	Name (void) const		{ return _name.c_str(); }
	inline uint32_t	Size (void) const			{ return _size; }
	inline dim_t	Width (void) const			{ return _w; }
	inline dim_t	Height (void) const			{ return _h; }
	inline EType	Type (void) const			{ return _type; }
	inline uint8_t	ThumbIndex (void) const			{ return _thumb; }
	inline void	SetThumbIndex (uint8_t i)		{ _thumb = i; }
	inline void	ResetThumbIndex (void)			{ _thumb = _type; }
    private:
	string		_name;
	uint32_t	_size;
	dim_t		_w,_h;
	EType		_type;
	uint8_t		_thumb;
    };
    using foldervec_t	= vector<CFolderEntry>;
    //}}}
private:
			DRAWFBDECL(ThumbCache);
    void		LoadFolder (void);
    void		LoadEntry (void);
    inline const CFolderEntry& CurEntry (void) const	{ return _files[_selection]; }
    inline void		OnImageViewKey (key_t key);
    inline void		OnFolderViewKey (key_t key);
    void		BeginThumbUpdate (void);
    static coord_t	CacheThumbX (unsigned thumbIndex)	{ return (thumbIndex % c_CacheNEntriesX) * c_ThumbWidth; }
    static coord_t	CacheThumbY (unsigned thumbIndex)	{ return (thumbIndex / c_CacheNEntriesY) * c_ThumbHeight; }
private:
    goid_t		_img;		///< Visible image in ImageView
    goid_t		_loadingImg;	///< Image currently being loaded. Not in _img to avoid drawing blank screen until loaded.
    goid_t		_vertices;	///< Specifications for selection rect and default file icons
    goid_t		_thumbs;	///< Texture containing the thumb cache
    goid_t		_thumbsDepth;	///< Depth texture attached to _thumbsFb
    goid_t		_thumbsFb;	///< The framebuffer used to draw onto _thumbs
    dim_t		_iw,_ih;	///< In ImageView specifies image dimensions
    float		_ix,_iy;	///< Top left of drawn image in screen pixels
    float		_iscale;	///< Zoom factor in ImageView
    foldervec_t		_files;		///< List of viewable files in current folder
    string		_selectionName;	///< Used to set selection after an up chdir and to load image file given as process arg
    EView		_view;		///< Current view. See EView above
    uint32_t		_selection;	///< Index into _files
    uint32_t		_firstentry;	///< Index of top left entry in view
    struct {				///< Thumb caching cycle parameters
	uint16_t	ThumbIndex;		///< Destination thumb slot
	uint16_t	FileIndex;		///< Index into _files
	uint16_t	ImageW, ImageH;		///< Dimensions of the loaded image
	goid_t		Image;			///< The loaded image
	uint32_t	LastThumbStamp;		///< Running counter for access stamps
	uint32_t	ThumbStamps [c_CacheNEntries];	///< LRU list for thumb cache
    }			_caching;
};

//----------------------------------------------------------------------
// Initialization and data loading

CImageViewer::CImageViewer (iid_t wid, const char* filename)
: CWindow (wid)
,_img (G::GoidNull)		// Can't load anything in the ctor, have to wait until OnInit
,_loadingImg (G::GoidNull)
,_vertices (G::GoidNull)
,_thumbs (G::GoidNull)
,_iw(0)
,_ih(0)
,_ix(0)
,_iy(0)
,_iscale(1.f)
,_files()
,_view (ImageView)
,_selection (0)
,_firstentry (0)
,_caching {0,0,0,0,G::GoidNull,0,{UINT32_MAX,UINT32_MAX,0}}	// First 2 ThumbStamps initialized to max to disable overwriting
{
    _selectionName = filename;	// Save filename to be selected in LoadEntry, called from OnInit
}

void CImageViewer::OnInit (void)
{
    CWindow::OnInit();

    // Here the long form of the Open call is used to make the viewer fullscreen.
    Open ("GLERI Image Viewer", WinInfo (0,0,800,600,0,0x33,0,WinInfo::MSAA_OFF,WinInfo::type_Normal,WinInfo::state_Fullscreen));
    TexParameter (G::Texture::MIN_FILTER, G::Texture::LINEAR);	// Enable linear texture filtering;
    TexParameter (G::Texture::MAG_FILTER, G::Texture::LINEAR);	//  it helps eliminate hard pixel boundaries.
    // Drawing and loading calls can be made henceforth

    // Primitives are always drawn from vertex buffers, created before
    // Draw can be called. These buffers must be uploaded to the GPU
    // memory before a shader running on the GPU can access them.
    static const coord_t c_Vertices[] = {
	// Each entry is drawn in a square with side c_EntryWidth. A square
	// is drawn as a two-triangle triangle strip, specified by four points.
	// Using the VGEN macro saves typing and offsetting complications.
	VGEN_TSRECT (0,0, c_EntryWidth,c_EntryWidth),
	// Second is the default folder thumbnail, looking like a paper folder.
	// Implemented as a line loop of 7 vertices in a 16x16 square.
	0,2, 0,15, 15,15, 15,2, 8,2, 7,0, 1,0,
	// Third is the default file thumbnail, simulating a paper sheet with
	// a bent corner. This is coded as a line loop, with 8 vertices.
	0,0, 0,15, 15,15, 15,2, 13,0, 13,2, 15,2, 13,0
    };
    _vertices = BufferData (G::ARRAY_BUFFER, c_Vertices, sizeof(c_Vertices));

    // Create the thumb cache.
    // For offscreen rendering, create a frambuffer on top of the
    // cache texture. OpenGL also requires a depth texture in the
    // framebuffer, even if it is not used.
    _thumbsDepth = CreateDepthTexture (c_CacheWidth, c_CacheHeight);
    _thumbs = CreateTexture (G::TEXTURE_2D, c_CacheWidth, c_CacheHeight, 0, G::Pixel::RGBA);
    _thumbsFb = CreateFramebuffer (_thumbsDepth, _thumbs);

    // Draw the default folder and file icons into the thumb cache, indexes 0 and 1
    _caching.ImageH = _caching.ImageW = 16;
    for (_caching.ThumbIndex = 0; _caching.ThumbIndex < 2; ++_caching.ThumbIndex)
	DrawThumbCache (_thumbsFb);

    // Load the default entry - may be a folder or an image
    LoadEntry();
}

void CImageViewer::LoadEntry (void)
{
    if (_selection >= _files.size() || CurEntry().Type() == CFolderEntry::Folder) {
	// A folder, or a fallback for an invalid _selection
	_view = FolderView;
	if (_selection < _files.size()) {
	    // Get the current dir name before chdir to find it in parent listing
	    char curdir [PATH_MAX], *pslash;
	    if (getcwd (curdir, sizeof(curdir)) && (pslash = strrchr(curdir,'/')))
		_selectionName = pslash+1;
	    chdir (CurEntry().Name());
	}
	LoadFolder();
	_selection = 0;
	_firstentry = 0;
	// Find the given filename in the directory listing.
	for (auto i = 0u; i < _files.size(); ++i) {
	    if (!strcmp (_selectionName.c_str(), _files[i].Name())) {
		_selection = i;
		// If it is an image file, load it. This only happens when this
		// file name was given as a command line arg at startup.
		if (_files[i].Type() == CFolderEntry::Image)
		    _loadingImg = LoadTexture (G::TEXTURE_2D, CurEntry().Name());
	    }
	}
	_selectionName.clear();
	OnKey (0);
    } else if (CurEntry().Type() == CFolderEntry::Image) {
	// Load the image into a texture object.
	if (_loadingImg != G::GoidNull)	// Check if already loading another image
	    return;
	// Textures can be loaded from raw data, from a datapak,
	// or from a file. This is the file version.
	_loadingImg = LoadTexture (G::TEXTURE_2D, CurEntry().Name());
    }
}

void CImageViewer::LoadFolder (void)
{
    _files.clear();
    _caching.FileIndex = UINT16_MAX;	// Stop thumb caching
    // List all subdirs and image files, loading entries into _files
    auto dd = opendir (".");
    for (struct dirent* de; (de = readdir (dd));) {
	struct stat st;
	if (access (de->d_name, R_OK) != 0 || stat (de->d_name, &st) < 0)
	    continue;						// Readable files only
	auto nlen = strlen(de->d_name);
	if ((!S_ISDIR(st.st_mode)				// List subfolders
	     || (de->d_name[0] == '.' && de->d_name[1] != '.'))	// Including "..", but not other hidden files
	    && (!S_ISREG(st.st_mode)				// Including regular files
		|| nlen <= 4					// With a recognizable extension
		|| (strcasecmp(de->d_name+nlen-4,".png")	// with extensions png,
		    && strcasecmp(de->d_name+nlen-4,".jpg")	// jpg, or jpeg
		    && strcasecmp(de->d_name+nlen-4,"jpeg"))))
	    continue;
	// Insert sorted by name.
	_files.emplace (lower_bound (_files.begin(), _files.end(), de->d_name),
			de->d_name, st.st_size, !S_ISDIR(st.st_mode));
    }
}

//----------------------------------------------------------------------
// Drawing. Both ImageView and FolderView drawn here, based on _view.

ONDRAWIMPL(CImageViewer)::OnDraw (Drw& drw) const
{
    CWindow::OnDraw (drw);
    if (_view == ImageView) {
	drw.Clear (color_ImageViewBackground);
	if (_img != G::GoidNull) {
	    drw.Scale (_iscale, _iscale);	// Scales all coordinates and sizes
	    // _ix is in screen pixels, so convert to image pixels for the draw.
	    drw.Image (_ix/_iscale, _iy/_iscale, _img);
	}
    } else { // _view == FolderView
	drw.Clear (color_FolderViewBackground);
	const coord_t filenameX = Font()->Width(), filenameY = c_EntryHeight-Font()->Height()*5/2;
	drw.Color (color_FolderViewText);
	// Bind the vertex pointer to _vertices to draw the selection rectangle
	drw.VertexPointer (_vertices);
	// Iterate over all visible entries
	for (auto y = 0u, ie = _firstentry; y <= (unsigned) Info().h-c_EntryHeight; y += c_EntryHeight) {
	    for (auto x = 0u; x <= (unsigned) Info().w-c_EntryWidth; ++ie, x += c_EntryWidth) {
		drw.Viewport (x,y,c_EntryWidth,c_EntryHeight);	// Set clipping rectangle to entry size. Clips long filenames.
		if (ie >= _files.size())
		    return;
		if (ie == _selection) {
		    drw.Color (color_FolderViewSelection);
		    drw.TriangleStrip (v_EntrySquareOffset, v_EntrySquareSize);
		    drw.Color (color_FolderViewText);
		}
		auto thumbIndex = _files[ie].ThumbIndex();	// Draw the thumbnail for the entry
		drw.Sprite (c_ThumbX, c_ThumbY, _thumbs, CacheThumbX (thumbIndex), CacheThumbY (thumbIndex), c_ThumbWidth, c_ThumbHeight);
		auto filename = _files[ie].Name();		// Draw the filename centered under the thumbnail
		drw.Text (max<coord_t> (filenameX, (c_EntryWidth-Font()->Width(filename))/2), filenameY, _files[ie].Name());
	    }
	}
    }
}

//----------------------------------------------------------------------
// Thumbnailer

// The thumbnailer is an asynchronous process. BeginThumbUpdate iterates
// over visible entries. When it comes to one without a thumbnail, it
// loads the image file. When the image file is loaded, OnTextureInfo
// event will arrive with image dimensions. At that point the thumbnail
// can be drawn onto _thumbs texture, and BeginThumbUpdate is called
// again. The cycle continues until all visible entries have a thumb.
void CImageViewer::BeginThumbUpdate (void)
{
    // If already loading a caching image, do nothing
    if (_caching.Image != G::GoidNull)
	return;
    unsigned pagesz = (Info().w/c_EntryWidth) * (Info().h/c_EntryHeight);
    pagesz = min<unsigned> (pagesz, _files.size()-_firstentry);
    // Find the first image without a thumb and initiate update from it
    for (auto i = _firstentry; i < _firstentry+pagesz; ++i) {
	if (_files[i].ThumbIndex() >= 2)
	    continue;	// Already have thumb
	if (_files[i].Type() != CFolderEntry::Image)
	    continue;	// Only images have thumbs
	// Find a free thumb slot using the LRU criterium.
	auto slot = 2u;
	for (auto s = 0u; s < ArraySize(_caching.ThumbStamps); ++s)
	    if (_caching.ThumbStamps[s] < _caching.ThumbStamps[slot])
		slot = s;
	// If slot is used, reset its users to default
	for (auto& f : _files)
	    if (f.ThumbIndex() == slot)
		f.ResetThumbIndex();
	// Take ownership of the slot and load the image
	_caching.ThumbStamps[slot] = ++_caching.LastThumbStamp;
	_caching.FileIndex = i;
	_caching.ThumbIndex = slot;
	_caching.Image = LoadTexture (G::TEXTURE_2D, _files[i].Name());
	// Done for now. When the image is loaded, the thumb will be created
	// in OnTextureInfo, and we'll return here.
	return;
    }
    // If did not return from the loop, then thumb update is finished. Draw.
    Draw();
}

// This generates the DrawThumbCache call for offscreen drawing into the
// _thumbsFb framebuffer. The code works exactly like OnDraw.
DRAWFBIMPL(CImageViewer,ThumbCache)
{
    drw.VertexPointer (_vertices);	// Contains the line art for the default icons
    drw.Viewport (CacheThumbX (_caching.ThumbIndex), CacheThumbY (_caching.ThumbIndex), c_ThumbWidth, c_ThumbHeight);
    drw.Clear (color_ThumbBackground);
    drw.Color (color_FolderViewText);
    auto scale = min (float(c_ThumbWidth)/_caching.ImageW, float(c_ThumbHeight)/_caching.ImageH);
    drw.Scale (scale, scale);		// Shrink the image to thumb dimensions
    if (_caching.ThumbIndex == CFolderEntry::Folder)
	drw.LineLoop (v_FolderIconOffset, v_FolderIconSize);
    else if (_caching.ThumbIndex == CFolderEntry::Image)
	drw.LineLoop (v_FileIconOffset, v_FileIconSize);
    else if (_caching.Image != G::GoidNull)
	drw.Image ((c_ThumbWidth/scale-_caching.ImageW)/2,	// Center it
		   (c_ThumbWidth/scale-_caching.ImageH)/2,	// in scaled coordinates, so /scale
		   _caching.Image);
}

//----------------------------------------------------------------------
// Window events

void CImageViewer::OnResize (dim_t w, dim_t h)
{
    CWindow::OnResize (w, h);
    OnKey ('m');	// In ImageView, remaximize the image. In FolderView, reposition selection and top entry.
}

// OnTextureInfo is generated when a texture finishes loading. Loading is
// initiated by LoadTexture call in LoadEntry. Other resource types may
// generate a similar event - LoadFont will generate OnFontInfo.
void CImageViewer::OnTextureInfo (goid_t tid, const G::Texture::Header& ih)
{
    CWindow::OnTextureInfo (tid, ih);
    if (tid == _loadingImg) {		// Image loaded for display in ImageView
	_iw = ih.w; _ih = ih.h;		// Save dimensions
	if (_img != G::GoidNull)	// Free old image, if present
	    FreeTexture (_img);
	_img = _loadingImg;		// Now showing the new image
	_loadingImg = G::GoidNull;
	_view = ImageView;		// Switch to ImageView, if not there already
    } else if (tid == _caching.Image) {	// Image loaded for thumbnailing
	_caching.ImageW = ih.w; _caching.ImageH = ih.h;	// Save dimensions for DrawThumbCache
	if (_caching.FileIndex < _files.size()) {	// If _files is still valid. LoadFolder resets FileIndex for this purpose.
	    DrawThumbCache (_thumbsFb);			// Draw to the thumbs cache texture
	    _files[_caching.FileIndex].SetThumbIndex (_caching.ThumbIndex);	// Set the thumb index in entry
	}
	FreeTexture (_caching.Image);	// The thumbnail is made, so the image is no longer need
	_caching.Image = G::GoidNull;
	BeginThumbUpdate();		// Resume thumbnailing visible entries
    }
    OnKey ('m');			// Remaximize the image and draw it
}

void CImageViewer::OnKey (key_t key)
{
    CWindow::OnKey (key);
    if (key == Key::Escape || key == 'q')
	return Close();
    // Route to view-specific handler below
    if (_view == ImageView)
	OnImageViewKey (key);
    else if (_view == FolderView)
	OnFolderViewKey (key);
    // Keys usually do something, so redraw
    Draw();
}

// OnKey for ImageView
inline void CImageViewer::OnImageViewKey (key_t key)
{
    if (key == Key::Left)			// Scrolling
	_ix += c_ScrollStep;
    else if (key == Key::Right)
	_ix -= c_ScrollStep;
    else if (key == Key::Up)
	_iy += c_ScrollStep;
    else if (key == Key::Down)
	_iy -= c_ScrollStep;
    else if (key == '0')			// Reset zoom to 1, image at natural size
	_iscale = 1.f;
    else if (key == 'm' && (_iw || _ih))	// Fit image to window. This is the default. Check iw,ih to avoid /0
	_iscale = min (float(Info().w)/_iw, float(Info().h)/_ih);
    else if (key == Key::Enter) {		// Enter returns to folder view
	_view = FolderView;
	OnKey (0);			// Scrolls the view to selection
    } else if (key == '+' || key == '-') {	// Changes the zoom factor
	auto mult = (key == '+' ? 2.f : 0.5f);
	_iscale *= mult;
	float scx = Info().w/2, scy = Info().h/2;
	_ix = (_ix-scx)*mult+scx;	// Keep image center at screen center
	_iy = (_iy-scy)*mult+scy;
    } else if (key == Key::Backspace && _loadingImg == G::GoidNull) {	// Load previous image in folder
	if (!_selection || _files[--_selection].Type() == CFolderEntry::Folder)
	    _view = FolderView;		// Stop if the previous item is a folder
	else
	    LoadEntry();
    } else if (key == Key::Space && _loadingImg == G::GoidNull) {	// Load next image in folder
	if (_selection >= _files.size()-1 || _files[++_selection].Type() == CFolderEntry::Folder)
	    _view = FolderView;		// Stop if no more items, or if it's a folder
	else
	    LoadEntry();
    }
    // Clamp the image position resulting from above to viewport
    auto sw = _iw*_iscale, sh = _ih*_iscale;
    if (sw > Info().w)
	_ix = min (0.f, max (_ix, Info().w - sw));
    else
	_ix = (Info().w - sw)/2;
    if (sh > Info().h)
	_iy = min (0.f, max (_iy, Info().h - sh));
    else
	_iy = (Info().h - sh)/2;
}

// OnKey for FolderView
inline void CImageViewer::OnFolderViewKey (key_t key)
{
    const unsigned linew = Info().w/c_EntryWidth,	// Compute how many entries can fit
		lineh = Info().h/c_EntryHeight,		// on a single page in this window
		pagesz = linew*lineh;
    if (!linew || !lineh)
	return;	// Window not mapped yet (happens because OnKey is explicitly called above to refit stuff)
    if (key == Key::Enter && _loadingImg == G::GoidNull)	// Enter loads the selected entry
	LoadEntry();						//  which can be an image or a folder
    else if (key == Key::Home)					// Navigation
	_selection = 0;
    else if (key == Key::End)
	_selection = _files.size()-1;
    else if (key == Key::Left && _selection > 0)
	--_selection;
    else if (key == Key::Right && _selection < _files.size()-1)
	++_selection;
    else if (key == Key::Up && _selection >= linew)
	_selection -= linew;
    else if (key == Key::Down && _selection < _files.size()-linew)
	_selection += linew;
    else if (key == Key::PageUp && _selection >= pagesz)
	_selection -= pagesz;
    else if (key == Key::PageDown && _selection < _files.size()-pagesz)
	_selection += pagesz;

    // Clip selection to view, or reposition view to selection
    if (_selection >= _files.size())
	_selection = 0;
    auto oldfirstentry = _firstentry;
    if (unsigned(_selection-_firstentry) >= pagesz)
	_firstentry = (_selection / pagesz) * pagesz;
    if (!_firstentry || oldfirstentry != _firstentry)	// If moved, something new might get exposed
	BeginThumbUpdate();				//  so run the thumbnailer
}

//----------------------------------------------------------------------
// The application object

class CImageViewerApp : public CGLApp {
    inline CImageViewerApp (void) : CGLApp(),_toOpen(nullptr) {}
public:
    static inline CImageViewerApp& Instance (void) {
	static CImageViewerApp s_App; return s_App;
    }
    void Init (argc_t argc, argv_t argv);
private:
    unique_c_ptr<char> _toOpen;
};

void CImageViewerApp::Init (argc_t argc, argv_t argv)
{
    CGLApp::Init (argc, argv);
    // With no args, default to current folder
    _toOpen = strdup (argc > 1 ? argv[1] : ".");
    // Check what it is. Could be a file or folder
    struct stat st;
    if (stat (_toOpen, &st) < 0) {
	perror (_toOpen);	// ... or a typo
	Quit();
	return;
    }
    // Chdir to it if a folder, otherwise chdir to its parent folder
    auto filename = "";
    if (!S_ISDIR(st.st_mode)) {
	auto pslash = strrchr (_toOpen.get(), '/');
	if (pslash) {
	    *pslash = 0;	// Split into dir and filename
	    filename = pslash+1;
	} else
	    filename = _toOpen;
    }
    chdir (_toOpen);
    CreateWindow<CImageViewer> (filename);
}

GLERI_APP (CImageViewerApp)
