// This is a "hello world" tutorial for GLERI, illustrating the simplest
// application that prints the traditional "Hello World!" message on
// screen and quits when a key is pressed. A good starting point.

//----------------------------------------------------------------------
// Step 1: include the library header, <gleri.h>
//	   ... but here use the local one, to allow compiling the
//	   tutorial executable before installing GLERI.
#include "../../gleri.h"

//----------------------------------------------------------------------
// Step 2: Define the main window class.
//
class CHelloWindow : public CWindow {
public:			// You must define at least 3 functions
    explicit		CHelloWindow (iid_t wid);
    virtual void	OnInit (void) override;
    ONDRAWDECL		OnDraw (Drw& drw) const; // Note the macro return type
			// To allow quitting, also recieve key events
    virtual void	OnKey (key_t key) override;
};

// The constructor mainly passes the window instance id to CWindow
CHelloWindow::CHelloWindow (iid_t wid)
: CWindow(wid)
{	// Note that the window is not created until after OnInit,
}	// so no drawing or resource creation commands can be sent here.

// Each window object requires some setup before it becomes functional.
// The application object will do the setup, and then call OnInit when
// communication with gleris becomes possible.
//
void CHelloWindow::OnInit (void)
{
    CWindow::OnInit();
    // OnInit must call Open to tell gleris to create a window.
    // Here, the first argument is the window title, followed by
    // dimensions. For windows that need more parameters you can
    // fill a WinInfo structure to pass to Open as the second argument.
    //
    Open ("Hello World", 320, 240);
    // Starting here you can create resources and call Draw()
}

// Note the macro beginning. OnDraw is actually a template, and is
// called twice. Once to compute the size of the drawlist, and the second
// time to write it. It is called from Draw(), which will package the
// drawlist into a message to send to gleris.
//
ONDRAWIMPL(CHelloWindow)::OnDraw (Drw& drw) const
{
    CWindow::OnDraw (drw);
    drw.Clear (RGB(0,0,64));		// Clear the screen. Color components can be 0-255
    drw.Color (RGB(128,128,128));	// Set drawing color
    static const char c_HelloMessage[] = "Hello world!";
    // Draw the hello message, centered on the screen.
    // Font() returns metric structure for the default font.
    // Info() returns the information structure for this window.
    drw.Text ((Info().w - Font()->Width (c_HelloMessage)) / 2,
	      (Info().h - Font()->Height()) / 2,
	      c_HelloMessage);
}

void CHelloWindow::OnKey (key_t key)
{
    CWindow::OnKey (key);
    if (key == Key::Escape || key == 'q')	// See gleri/event.h for more key codes
	Close();			// Closing the last window quits the application
}

//----------------------------------------------------------------------
// Step 3: Define the application object
//
class CHello : public CGLApp {
    inline CHello (void) : CGLApp() {}
public:
    // Application objects are singletons, meaning that they ensure
    // that only one application object can exist. Defining an Instance
    // function like this and making the constructor private is the way
    // to implement that. The function name is important: it's what
    // main will call.
    static inline CHello& Instance (void) {
	static CHello s_App; return s_App;
    }
    // Init is called from main() once before the main loop, with the args.
    void Init (argc_t argc, argv_t argv) {
	CGLApp::Init (argc, argv);
	CreateWindow<CHelloWindow>();	//< This creates the hello window
    }	// The application object retains ownership of the window object
};

//----------------------------------------------------------------------
// Step 4: Include the GLERI_APP macro that implements a main() creating and
//	   calling your application object and its main event loop.
//
GLERI_APP (CHello)
