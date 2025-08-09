#pragma once
#include "Tanita2.h"
#include "FileManager.h"
#include "Graphics.h"
#include "Sound.h"
#include "Input.h"
#include "Python.h"
#include "Config.h"

//! Application and window management class.
class ApplicationInstance
{
public:
    //! Initializations.
    /** Order of initializations: file manager, python, 
      * configuration database, application window, direct3d,
      * input system. Then init() method is called. */
    ApplicationInstance();
    //! Resource releasing and deinitialization.
    /** Calls "on_cleanup" user script function. All systems
      * are released implicitly via their destructors. */
    ~ApplicationInstance();

    //! Game loop.
    /** Processes window messages, then updates all managers and 
      * calls on_frame() method for actual game data update and rendering. */
    void run();
    
    //! Process window messages
    /** Returns false if WM_QUIT message retrieved */
    static bool process_messages();
    
    //! Pause game
    inline static void pause( bool pause = true )
        { paused = pause;
          if (!paused) previous_ticks = GetTickCount(); }
    //! Resume game
    inline static void resume() { pause(false); }
    
    //! Key press handler
    /** @param  keycode  virtual key code
      * @param  pressed  true if key was pressed, false if released */
    void on_key_press( int keycode, bool pressed = true );
    //! Window activation handler
    /** @param  become_active  true if window became active, otherwise false */
    void on_activate( bool become_active );

    //! Function called when scripts are being reloaded
    static void on_script_reload();

    //! Screen repainting handler
    /** WM_PAINT event handler */
    inline void on_repaint() { on_frame(0, true); }
    
    //! Disable window automatic activation
    static inline void disable_autoactivation( bool disable )
	{
		ApplicationInstance::disable_activation = disable; 
	}
        
    //! Onmouse window activation flag
    static bool disable_activation;
    
    //! Display python traceback message box
    /** @param  traceback  traceback message to display */
    static void display_traceback( const std::string & traceback );
    
    //! Handle of the main application window.
    static HWND window;
    
    //! Cursor position
    static D3DXVECTOR2 cursor_position;
    //! Mouse button state
    static int mouse_button_state;
    
    //! Window activity flag (true if window is active)
    static bool active;
    
    //! Game pause flag
    static bool paused;
    
    //! Game is to be quit flag
    static bool quit_game;
    
    //! Flag indicating that window may use application class
    static bool window_may_use_app;
    
    //! Time since application start in milliseconds
    DWORD application_time;
    
    //! Time delta
    DWORD application_dt;
    
    //! Miscellaneous initializations.
    /** Called at the end of constructor function
      * after initializations of all systems */
    void init();
    
    //! High-level game engine update and rendering function.
    /** \param  dt               time step
      * \param  just_redraw      true if no script update is needed */
    void on_frame( DWORD dt, bool just_redraw = false );
    
protected:
    //! Check if Direct3D devices were lost
    /** \return true if no devices are lost */
    bool check_lost_devices();

    //! States of all keyboard keys
    static bool key_states[256];
    
    /* Undocumented properties */
    
    // File manager
    SINGLETON(FileManager) file_manager;
    // DirectInput manager
    SINGLETON(DirectInput) input;
    // Direct3D manager
    SINGLETON(Direct3D) direct3d;
    // DirectSound manager
    SINGLETON(DirectSound) directsound;
    // Python interpreter
    SINGLETON(python::Python) python;
    // Configuration database
    SINGLETON(Config) config;
    
    // Python on_frame function object
    bp::object py_on_frame;
    
    // Previous time
    static DWORD previous_ticks;
};
//! Application singleton definition
typedef Singleton<ApplicationInstance> Application;

//! Windows application instance handle
extern HINSTANCE hInstance;
