#pragma once
#include "Exceptions.h"
#include "Templates.h"
#include "Tanita2.h"
#define DIRECTINPUT_VERSION DIRECTINPUT_HEADER_VERSION
#include <dinput.h>
#undef DIRECTINPUT_VERSION

//! User input handler class
class DirectInputInstance
{
public:
    //! Constructor
    /** Initializes DirectInput and acquires mouse device */
    DirectInputInstance();
    //! Destructor
    /** Releases all DirectInput resources */
    ~DirectInputInstance();

    //! Acquires mouse
    /** Called when application window becomes active */
    void acquire();
    //! Unacquires mouse to let other applications use it
    /** Called when application window becomes inactive */
    inline void unacquire() { ASSERT_DIRECTX(mouse->Unacquire()); }
    
    //! Update input state
    /** Processes mouse events and updates
      * ApplicationInstance::cursor_position and 
      * ApplicationInstance::mouse_button_state variables */
    void update( DWORD dt );
    
    //! Check if device was lost and restore if possible
    bool is_lost();

protected:
    // DirectInput instance
    LPDIRECTINPUT8 direct_input;
    // Mouse object
    LPDIRECTINPUTDEVICE8 mouse;
    // Cached window width and height
    int width, height;
    
    // Internal (not clipped) cursor position
    int x, y;
    
    // Mouse device is lost
    bool is_device_lost;
};
// DirectInput singleton definition
typedef Singleton<DirectInputInstance> DirectInput;
