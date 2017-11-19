#include "../gleri/app.h"
#include "../gleri/pak.h"
#include <sys/stat.h>

class CGleriPakApp : public CApp {
    inline		CGleriPakApp (void);
public:
    inline static auto&	Instance (void) { static CGleriPakApp s_App; return s_App; }
    static void		PrintUsageAndExit (void) NORETURN;
    inline void		Init (argc_t argc, argv_t argv);
    inline int		Run (void);
private:
    inline void		ReadAllInputFiles (pakbuf_t& outbuf) const;
    inline void		ExtractPakbufContents (const pakbuf_t& outbuf) const;
    inline void		WriteAsSourceCode (const pakbuf_t& outbuf) const;
    inline void		WriteBinaryArchive (const pakbuf_t& outbuf) const;
private:
    string		_outfilename;
    string		_varname;
    string		_prefix;
    vector<string>	_infiles;
    bool		_bCOutput:1;
    bool		_bUncompressed:1;
    bool		_bExtract:1;
};

CGleriPakApp::CGleriPakApp (void)
: CApp()
,_outfilename()
,_varname()
,_prefix()
,_infiles()
,_bCOutput (false)
,_bUncompressed (false)
,_bExtract (false)
{
}

void CGleriPakApp::PrintUsageAndExit (void) // static
{
    fputs (
	"gleripak " GLERI_VERSTRING "\n\n"
	"Manipulates gleri .pak archives, used by the RGL::LoadDatapak API.\n\n"
	"Usage: gleripak [-bcnvx] archive.pak [FILE...]\n\n"
	"Options:\n"
	"  -b <base>  Remove given prefix from archived filenames.\n"
	"  -c   Output the archive as a C++ source file with array.\n"
	"  -n   Do not compress added files.\n"
	"  -v   Set the name of the C++ array variable. Implies -c.\n"
	"  -x   Extract the archive.\n"
	, stderr);
    exit (EXIT_FAILURE);
}

void CGleriPakApp::Init (argc_t argc, argv_t argv)
{
    CApp::Init (argc, argv);
    for (int o; 0 < (o = getopt (argc, argv, "b:cntv:"));) {
	if (o == 'b')		_prefix = optarg;
	else if (o == 'c')	_bCOutput = true;
	else if (o == 'n')	_bUncompressed = true;
	else if (o == 'v')	{ _varname = optarg; _bCOutput = true; }
	else if (o == 'x')	_bExtract = true;
	else			PrintUsageAndExit();
    }
    if (optind >= argc
	    || !argv[optind][0]	// no output name given
	    || (_bExtract && optind+1 != argc))
	PrintUsageAndExit();

    // Save filenames
    _outfilename = argv[optind];
    for (auto i = optind+1; i < argc; ++i)
	_infiles.emplace_back (argv[i]);

    // Validity checks on filenames
    if (_bExtract || _infiles.empty()) {	// extraction and testing
	if (0 != access (_outfilename.c_str(), R_OK))
	    throw XError ("archive '%s' is not readable", _outfilename.c_str());
    } else {
	for (const auto& f : _infiles)
	    if (0 != access (f.c_str(), R_OK))
		throw XError ("input file '%s' not found", f.c_str());
    }

    // Check if C++ output is requested
    #define CPP_EXTENSION	".cc"
    if (_outfilename.size() > strlen(CPP_EXTENSION)
	    && _outfilename.size()-strlen(CPP_EXTENSION) == _outfilename.rfind (CPP_EXTENSION))
	_bCOutput = true;
    // Set varname if unspecified
    if (_bCOutput && _varname.empty()) {
	_varname = _outfilename;
	// Basename only
	auto slashpos = _varname.rfind ('/');
	if (slashpos != string::npos)
	    _varname.erase (0, slashpos+1);
	_varname.erase (_varname.rfind ('.'), string::npos);
	_varname.insert (0, "File_");
    }
}

int CGleriPakApp::Run (void)
{
    pakbuf_t outbuf;
    if (_infiles.empty()) {
	//
	// Reading an existing archive
	//
	outbuf = ReadFileToPakbuf (_outfilename.c_str());
	if (outbuf.empty())
	    throw XError ("error reading archive '%s': %s", _outfilename.c_str(), strerror(errno));
	if (PakbufIsCompressed (outbuf)) {
	    outbuf = DecompressPakbuf (outbuf);
	    if (outbuf.empty())
		throw XError ("decompression of archive '%s' failed", _outfilename.c_str());
	}
	ExtractPakbufContents (outbuf);
    } else {
	//
	// Creating a new archive
	//
	ReadAllInputFiles (outbuf);

	// Add trailer termination
	FinishPakbuf (outbuf);
	// Compress, unless told not to
	if (!_bUncompressed)
	    outbuf = CompressPakbuf (outbuf);

	// Write output archive
	if (_bCOutput)
	    WriteAsSourceCode (outbuf);
	else
	    WriteBinaryArchive (outbuf);
    }
    return EXIT_SUCCESS;
}

void CGleriPakApp::ReadAllInputFiles (pakbuf_t& outbuf) const
{
    // Add all given files
    for (const auto& f : _infiles) {
	auto infbuf = ReadFileToPakbuf (f.c_str());
	if (infbuf.empty())
	    throw XError ("error reading input file '%s': %s", f.c_str(), strerror(errno));
	auto nameinpak = f;
	if (!_prefix.empty() && 0 == nameinpak.find(_prefix))
	    nameinpak.erase (0, _prefix.size());
	AddFileToPakbuf (outbuf, nameinpak.c_str(), infbuf);
    }
}

static void writefd (int fd, const char* buf, size_t bufsz)
{
    while (bufsz) {
	auto bw = write (fd, buf, bufsz);
	if (bw <= 0 && errno != EINTR)
	    throw XError ("write error: %s", strerror(errno));
	buf += bw;
	bufsz -= bw;
    }
}

static void writefile (const char* filename, const char* buf, size_t bufsz)
{
    // Create parent directories, if needed
    char fnpath [PATH_MAX];
    fnpath[1] = 0;
    snprintf (ArrayBlock(fnpath), "%s", filename);
    for (char* pdir = fnpath+1; *pdir; ++pdir) {
	if (*pdir == '/') {
	    *pdir = 0;
	    if (0 > mkdir (fnpath, S_IRWXU) && errno != EEXIST)
		throw XError ("failed to create directory '%s': %s", fnpath, strerror(errno));
	    *pdir = '/';
	}
    }

    int fd = open (filename, O_WRONLY| O_CREAT| O_TRUNC, DEFFILEMODE);
    if (fd < 0)
	throw XError ("failed to create '%s': %s", filename, strerror(errno));
    writefd (fd, buf, bufsz);
    if (0 > close (fd))
	throw XError ("failed to close '%s': %s", filename, strerror(errno));
}

void CGleriPakApp::ExtractPakbufContents (const pakbuf_t& outbuf) const
{
    auto ife = PakbufBegin (outbuf);
    const auto ifeend = PakbufEnd (outbuf);
    const auto outbufend = &*outbuf.end();
    for (; ife < ifeend; ife = ife->Next()) {
	auto fsz = ife->Filesize();
	auto fn = ife->Filename();
	auto fdp = ife->Filedata();
	if (ife->Namesize() > outbufend - reinterpret_cast<pakbuf_t::const_pointer>(fn)
		|| fsz > outbufend - fdp
		|| ife->Namesize() == 0 || ife->Namesize() > PATH_MAX
		|| fn[ife->Namesize()-1] != 0)
	    XError::emit ("file data corrupted");
	if (0 == strcmp (fn, CPIO_TRAILER_FILENAME))
	    continue;
	if (_bExtract)
	    writefile (fn, reinterpret_cast<const char*>(fdp), fsz);
	else
	    printf ("%u\t%s\n", fsz, fn);
    }
    if (ife != ifeend)
	XError::emit ("file data corrupted");
}

void CGleriPakApp::WriteAsSourceCode (const pakbuf_t& outbuf) const
{
    int ofd = STDOUT_FILENO;
    if (_outfilename != "-") {	// "-" specifies writing to stdout
	ofd = open (_outfilename.c_str(), O_WRONLY| O_CREAT| O_TRUNC, DEFFILEMODE);
	if (ofd < 0)
	    throw XError ("error creating archive '%s': %s", _outfilename.c_str(), strerror(errno));
    }

    // Write the data
    char line [256];
    auto linesz = snprintf (ArrayBlock(line),
			"//{""{{ %s\n"
			"extern \"C\" const unsigned int %s_size = %zu;\n"
			"extern \"C\" const unsigned char %s [%zu] = {\n",
			_varname.c_str(),
			_varname.c_str(), outbuf.size(),
			_varname.c_str(), outbuf.size());
    writefd (ofd, line, linesz);
    for (size_t i = 0; i < outbuf.size();) {
	linesz = 0;
	line[linesz++] = '\t';
	for (size_t j = 0; j < 16 && i < outbuf.size(); ++j, ++i)
	    linesz += snprintf (&line[linesz], ArraySize(line)-linesz, "%hhu,", outbuf[i]);
	linesz -= (i == outbuf.size());	// remove last comma
	line[linesz++] = '\n';
	writefd (ofd, line, linesz);
    }
    linesz = snprintf (ArrayBlock(line), "};\n//""}}}-------------------------------------------------------------------\n");
    writefd (ofd, line, linesz);

    // Close, if not stdout
    if (ofd != STDOUT_FILENO && 0 > close (ofd))
	throw XError ("error closing archive '%s': %s", _outfilename.c_str(), strerror(errno));

    // Write a header if have a .cc file
    if (_outfilename.size() > strlen(CPP_EXTENSION)
	    && _outfilename.size()-strlen(CPP_EXTENSION) == _outfilename.rfind (CPP_EXTENSION)) {
	auto headername = _outfilename;
	headername.replace (headername.size()-strlen(CPP_EXTENSION), strlen(CPP_EXTENSION), ".h");
	int hfd = open (headername.c_str(), O_WRONLY| O_CREAT| O_TRUNC, DEFFILEMODE);
	if (hfd < 0)
	    throw XError ("error creating header '%s': %s", headername.c_str(), strerror(errno));
	linesz = snprintf (ArrayBlock(line),
			"extern \"C\" const unsigned int %s_size;\n"
			"extern \"C\" const unsigned char %s [%zu];\n",
			_varname.c_str(),
			_varname.c_str(), outbuf.size());
	writefd (ofd, line, linesz);
	if (0 > close (hfd))
	    throw XError ("error closing header '%s': %s", headername.c_str(), strerror(errno));
    }
}

void CGleriPakApp::WriteBinaryArchive (const pakbuf_t& outbuf) const
{
    int wok;
    if (_outfilename == "-")
	wok = WritePakbufToFd (outbuf, STDOUT_FILENO);
    else
	wok = WritePakbufToFile (outbuf, _outfilename.c_str());
    if (wok != 0)
	throw XError ("error writing archive '%s': %s", _outfilename.c_str(), strerror(errno));
}

GLERI_APP (CGleriPakApp)
