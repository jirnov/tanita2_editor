#include "stdafx.h"
#include "Input.h"
#include "Application.h"

// Application main window handle
extern HWND toplevel_window;

// Constructor
DirectInputInstance::DirectInputInstance()
     : direct_input(NULL), mouse(NULL), x(0), y(0), is_device_lost(false)
{
    // Creating DirectInput object
    ASSERT_DIRECTX(DirectInput8Create(hInstance, DIRECTINPUT_HEADER_VERSION,
                                      IID_IDirectInput8, (void **)&direct_input, NULL));
    // Creating mouse handle
    ASSERT_DIRECTX(direct_input->CreateDevice(GUID_SysMouse, &mouse, NULL));
    
    // Initializations
    Config cfg;
    width = cfg->get<int>("width");
    height = cfg->get<int>("height");
    ASSERT_DIRECTX(mouse->SetDataFormat(&c_dfDIMouse));

  #define INPUT_MODE DISCL_BACKGROUND | DISCL_NONEXCLUSIVE
    
  #define MainWindow ApplicationInstance::window
    ASSERT_DIRECTX(mouse->SetCooperativeLevel(MainWindow, INPUT_MODE));
#undef INPUT_MODE
    DIPROPDWORD MProp;
    MProp.diph.dwSize = sizeof(DIPROPDWORD);
    MProp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    MProp.diph.dwObj = 0;
    MProp.diph.dwHow = DIPH_DEVICE;
    MProp.dwData = 64;
    ASSERT_DIRECTX(mouse->SetProperty(DIPROP_BUFFERSIZE, &MProp.diph));
}

// Destructor
DirectInputInstance::~DirectInputInstance()
{
#define SAFE_RELEASE(res) { if (res) res->Release(); }
    if (mouse) mouse->Unacquire();
    SAFE_RELEASE(mouse);
    SAFE_RELEASE(direct_input);
#undef SAFE_RELEASE
}

// Acquiring mouse
void DirectInputInstance::acquire()
{
    HRESULT result = mouse->Acquire();
    if (DIERR_OTHERAPPHASPRIO != result)
    {
        ASSERT_DIRECTX(result);
    }
    else
        is_device_lost = true;
}

// Check if device was lost and restore if possible
bool DirectInputInstance::is_lost()
{
    if (!is_device_lost)
        return false;
    is_device_lost = false;
    acquire();
    return is_device_lost;
}

// Update input state
void DirectInputInstance::update( DWORD dt )
{
    DWORD elements = 1;
    DIDEVICEOBJECTDATA data;
    
    ApplicationInstance::mouse_button_state &= 7;
    while (true)
    {
        if (FAILED(mouse->GetDeviceData(sizeof(data), &data, &elements, 0)))
        {
            acquire();
            break;
        }
        if (elements == 0)
            break;
        switch(data.dwOfs)
        {
        case DIMOFS_X:
            x += data.dwData;
            break;

        case DIMOFS_Y:
            y += data.dwData;
            break;
            
        case DIMOFS_Z:
            ApplicationInstance::mouse_button_state |= 1 << (int(data.dwData) > 0 ? 3 : 4);
            break;

        case DIMOFS_BUTTON0:
        case DIMOFS_BUTTON1:
        case DIMOFS_BUTTON2:
        case DIMOFS_BUTTON3:
        case DIMOFS_BUTTON4:
        case DIMOFS_BUTTON5:
        case DIMOFS_BUTTON6:
        case DIMOFS_BUTTON7:
            int button = data.dwOfs - DIMOFS_BUTTON0;
            if (data.dwData & 0x80)
                ApplicationInstance::mouse_button_state |= 1 << button;
            else
                ApplicationInstance::mouse_button_state &= ~(1 << button);
            break;
        }
    }
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(ApplicationInstance::window, &p);
    ApplicationInstance::cursor_position = D3DXVECTOR2(float(p.x), float(p.y));
    
    ApplicationInstance::cursor_position.x *= 1024L;
    ApplicationInstance::cursor_position.x /= width;
    
    ApplicationInstance::cursor_position.y *= 768L;
    ApplicationInstance::cursor_position.y /= height;
}
