#pragma once
#include "config.h"

//----------------------------------------------------------------------

class CMMFile {
public:
    typedef unsigned char	value_type;
    typedef unsigned		size_type;
    typedef value_type*		pointer;
    typedef const value_type*	const_pointer;
public:
				CMMFile (void)			:_fd(-1),_sz(0),_p(nullptr) {}
    inline explicit		CMMFile (const char* filename)	{ Open (filename); }
    inline			~CMMFile (void)			{ Close(); }
    void			Open (const char* filename);
    void			Close (void) noexcept;
    inline const_pointer	Data (void) const		{ return (_p); }
    inline size_type		Size (void) const		{ return (_sz); }
    static pointer		DecompressBlock (const_pointer p, size_type isz, size_type& osz);
private:
    int				_fd;
    size_type			_sz;
    pointer			_p;
};
