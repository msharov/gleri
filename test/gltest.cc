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
	CGLApp::Init (argc, argv, server_Pipe);
	CreateWindow<CTestWindow>();
    }
};

GLERI_APP (CGLTest)
