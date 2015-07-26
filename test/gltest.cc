// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "twin.h"

class CGLTest : public CGLApp {
public:
    inline CGLTest (void) : CGLApp() {}
    static inline CGLTest& Instance (void) {
	static CGLTest s_App; return s_App;
    }
    void Init (argc_t argc, argv_t argv) {
	auto st = server_Pipe;
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'c')
	    st = server_Local;
	CGLApp::Init (argc, argv, st);
	CreateWindow<CTestWindow>();
    }
};

GLERI_APP (CGLTest)
