#include "twin.h"

class CGLTest : public CGLApp {
public:
    inline CGLTest (void) : CGLApp() {}
    static inline CGLTest& Instance (void) {
	static CGLTest s_App; return (s_App);
    }
    void Init (argc_t argc, argv_t argv) {
	CGLApp::Init (argc, argv);
	CreateWindow<CTestWindow>();
    }
};

GLERI_APP (CGLTest)
