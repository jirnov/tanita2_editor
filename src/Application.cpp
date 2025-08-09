#include "stdafx.h"
#include "Application.h"
#include "Log.h"
#include "resource/resource.h"
#pragma warning(disable: 4311)

// Application rendering window
HWND ApplicationInstance::window;
// Application activity flag
bool ApplicationInstance::active = true;
// Game pause flag
bool ApplicationInstance::paused = false;
// Key states
bool ApplicationInstance::key_states[256];
// Cursor position
D3DXVECTOR2 ApplicationInstance::cursor_position;
// Mouse button state
int ApplicationInstance::mouse_button_state;
// Flag indicating that window may use application class
bool ApplicationInstance::window_may_use_app;
// Game is to be quit flag
bool ApplicationInstance::quit_game = false;
// Previous time value
DWORD ApplicationInstance::previous_ticks = GetTickCount();

// Window function
LRESULT CALLBACK WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

//! Onmouse window activation flag
bool ApplicationInstance::disable_activation;

// Creates window and initializes other systems.
#define CLASS_NAME "Tanita2GameApplication"
ApplicationInstance::ApplicationInstance()
    : application_time(0), application_dt(0)
{
    // Python initialization
    python.create();
    // Configuration database initialization
    config.create();
    // File manager initialization
    file_manager.create();
    
    // Class registration
    Config config;
    Log log;
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_CLASSDC;
    wcex.lpfnWndProc    = (WNDPROC)WindowProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, (LPCTSTR)IDI_TANITA2);
    wcex.hCursor        = NULL;
    wcex.hbrBackground  = NULL;
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = CLASS_NAME;
    wcex.hIconSm        = LoadIcon(hInstance, (LPCTSTR)IDI_SMALL);
    ASSERT_WINAPI(RegisterClassEx(&wcex));

    // Creating window
    #define WINDOW_POSTFIX " - Editor"
    #define BORDER_STYLE WS_BORDER
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | 
                  WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BORDER_STYLE;
    RECT r = {0, 0, config->get<int>("width"), config->get<int>("height")};
    // Getting needed window size
    AdjustWindowRect(&r, style, FALSE);
    int x = config->get<int>("x"), y = config->get<int>("y"),
        w = r.right - r.left + 1,  h = r.bottom - r.top + 1;
    if (x != CW_USEDEFAULT && x + r.left >= 0) x += r.left;
    if (y != CW_USEDEFAULT && y + r.top >= 0)  y += r.top;
    config->set<int>("x", x);
    config->set<int>("y", y);
    config->set<int>("actual_width", w);
    config->set<int>("actual_height", h);
    window = CreateWindow(wcex.lpszClassName, "Tanita2" WINDOW_POSTFIX,
                          style, x, y, w, h, NULL, NULL, hInstance, NULL);
#undef WINDOW_POSTFIX
    ASSERT_WINAPI(window);
    ShowWindow(window, SW_SHOWNORMAL);
    UpdateWindow(window);

    // Initializing Direct3D
    direct3d.create();
    
    // Initializing DirectSound
    directsound.create();
    
    // Initializing DirectInput
    mouse_button_state = 0;
    input.create();
}

// Process window messages
bool ApplicationInstance::process_messages()
{
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        
        if (WM_QUIT == msg.message)
            return false;
    }
    return true;
}

// Game loop
void ApplicationInstance::run()
{
    // Notifying window that application was initialized
    window_may_use_app = true;

    while (process_messages() && !quit_game)
    {
        // Updating timers and calculating dt
        DWORD ticks = GetTickCount();
        DWORD dt = ticks - previous_ticks;
        
        // Checking if Direct3D devices were lost
        if (check_lost_devices())
            continue;
        
        // Incrementing application time
        application_time += dt;
        application_dt = dt;
        
        // Updating input system
        if (active)
            input->update(dt);
        // Updating other systems
        direct3d->update(dt);
        file_manager->update(dt);

        // Updating window by timer in editor mode
#define TIMER_PERIOD 100
        static DWORD last_update_time;
        if (!(active || application_time - last_update_time > TIMER_PERIOD))
            continue;
        else last_update_time = application_time,
        // Saving time
        previous_ticks = ticks;

        // Update and drawing on each frame
        on_frame(dt);
        
        directsound->update(dt);
    }
    // Notifying window that application is to be destroyed
    window_may_use_app = false;

}

//! Window function.
/** Game window message processing. */
LRESULT CALLBACK WindowProc( HWND hWnd, UINT message, 
                             WPARAM wParam, LPARAM lParam )
{
    bool flag = false;
    
    switch (message)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
		break;
    case WM_KEYUP:
        flag = true;
    case WM_KEYDOWN:
        if (ApplicationInstance::window_may_use_app)
        {
            Application app;
            app->on_key_press((int)wParam, !flag);
        }
        return 0;
    case WM_MOUSEMOVE:
        if (ApplicationInstance::window_may_use_app && !ApplicationInstance::disable_activation)
        {
            SetForegroundWindow(ApplicationInstance::window);
            ApplicationInstance::active = true;
        }
        return 0;
    case WM_SETCURSOR:
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        break;
    case WM_TIMER:
    case WM_PAINT:
        if (ApplicationInstance::window_may_use_app)// && !ApplicationInstance::disable_activation)
        {
           Application app;
           app->on_repaint();
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_ACTIVATE:
        ApplicationInstance::active = LOWORD(wParam) != WA_INACTIVE;
        if (ApplicationInstance::window_may_use_app)
        {
            Application app;
            app->on_activate(ApplicationInstance::active);
        }
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Window activation handler
void ApplicationInstance::on_activate( bool become_active )
{
    ZeroMemory(key_states, sizeof(key_states));
    // In editor mode application doesn't need to (un)acquire mouse
    mouse_button_state = 0;
}

// Check if Direct3D devices were lost
bool ApplicationInstance::check_lost_devices()
{
    Direct3D d3d;
    DirectInput input;
    return d3d->is_lost() || input->is_lost();
}

// Keyboard input handler
void ApplicationInstance::on_key_press( int keycode, bool pressed )
{
    python::Python py;
    
    // Calling engine key processing function
    if (pressed && !key_states[keycode])
        bp::call<void>(bp::object(py["Lib"].attr("engine").attr("on_keypress")).ptr(), keycode);
    key_states[keycode] = pressed;
    
    // Reloading on Ctrl+R
    if (key_states[VK_CONTROL] && key_states[0x52 /* R */])
        bp::call<void>(bp::object(py["Lib"].attr("engine").attr("on_reload")).ptr());
    
    // Movement arrow keys processing
    if (pressed && VK_LEFT <= keycode && VK_DOWN >= keycode)
    {
        int xdir = keycode == VK_LEFT ? -1 : keycode == VK_RIGHT ? 1 : 0;
        int ydir = keycode == VK_UP   ? -1 : keycode == VK_DOWN  ? 1 : 0;
        bp::call<void>(bp::object(py["Lib"].attr("engine").attr("on_move_request")).ptr(),
                           xdir, ydir, key_states[VK_SHIFT]);
    }
}

// Called on script reloading
void ApplicationInstance::on_script_reload()
{
    Direct3D d3d;
    d3d->clear_render_queue();
    
    DirectSound dsound;
    dsound->clear_render_queue();
}

// Local initialization
void ApplicationInstance::init()
{
    // Setting timer to redraw
    SetTimer(window, 1, TIMER_PERIOD, NULL);
  #undef TIMER_PERIOD

    // Getting script engine on_frame function bp::object
    python::Python py;
    py_on_frame = py["Lib"].attr("engine").attr("on_frame");
   
    // Initializing python engine
	try
	{
		bp::call<void>(bp::object(py["Lib"].attr("engine").attr("on_init")).ptr(), long(window));
	}
	catch(bp::error_already_set &)
	{
		PyErr_Print();
		py->traceback();
		throw;
	}
    
    BringWindowToTop(window);
    SetActiveWindow(window);
}

// Update and redraw
void ApplicationInstance::on_frame( DWORD dt, bool just_redraw )
{
	if (ApplicationInstance::disable_activation)
		return;

	python::Python py;
    // Updating game state
	try
	{
	    bp::call<void>(py_on_frame.ptr(), dt / 1000.0f, just_redraw, cursor_position, mouse_button_state);
		direct3d->zoom = bp::extract<float>(py["Lib"].attr("Globals").attr("zoom"));
	}
	catch(bp::error_already_set &)
	{
		PyErr_Print();
		py->traceback();
		throw;
	}

    // Rendering
    direct3d->clear(D3DCOLOR_XRGB(255, 255, 0));
    direct3d->present();
}

// Resource releasing and deinitialization.
ApplicationInstance::~ApplicationInstance()
{
    window_may_use_app = false;
    
	// Cleaning python engine
	python::Python py;
    try
    {
    	bp::call<void>(bp::object(py["Lib"].attr("engine").attr("on_cleanup")).ptr());    
    }
    catch(bp::error_already_set &)
    {
        PyErr_Print();
        py->traceback();
        throw;
    }

    // Miscellaneous cleanup
    UnregisterClass(CLASS_NAME, hInstance);
}
#undef CLASS_NAME

// Display python traceback message box
void ApplicationInstance::display_traceback( const std::string & traceback ) {}
