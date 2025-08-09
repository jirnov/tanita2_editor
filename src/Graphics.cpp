#include "stdafx.h"
#include "TextureManager.h"
#include "VertexManager.h"
#include "SequenceManager.h"
#include "Application.h"
#include "Graphics.h"
#include "Config.h"
#include "Log.h"
#include <d3dx9.h>

// Save screenshot flag
bool Direct3DInstance::need_save_screenshot = false;

// Constructor
Direct3DInstance::Direct3DInstance()
    : is_device_lost(false), rtt_enabled(false),
      hardware_yuv_enabled(false)
{
    Log log;
    Config conf;
    
    // Creating Direct3D object
    if (NULL == (d3d = Direct3DCreate9(D3D_SDK_VERSION)))
        throw Exception("Direct3D object creation failed");

    // Preparing device	parameters
    D3DPRESENT_PARAMETERS & pp = present_params;
    ZeroMemory(&pp, sizeof(pp));
    pp.Windowed = conf->get<bool>("windowed");
    if (!pp.Windowed)
        pp.FullScreen_RefreshRateInHz = conf->get<int>("refresh_rate");
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferCount = 1;
    pp.BackBufferFormat = pp.Windowed ? D3DFMT_UNKNOWN : D3DFMT_X8R8G8B8;
    pp.BackBufferWidth = conf->get<int>("width");
    pp.BackBufferHeight = conf->get<int>("height");
    pp.EnableAutoDepthStencil = false;
    pp.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
    pp.hDeviceWindow = ApplicationInstance::window;
    pp.MultiSampleType = D3DMULTISAMPLE_NONE;
    pp.PresentationInterval = conf->get<bool>("vertical_sync") ? D3DPRESENT_INTERVAL_ONE : 
                                                                 D3DPRESENT_INTERVAL_IMMEDIATE;
    ASSERT_DIRECTX(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                       pp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                       &pp, &device));
    // Getting device capabilities
    ASSERT_DIRECTX(device->GetDeviceCaps(&device_caps));
    
    // Printing device information
    D3DADAPTER_IDENTIFIER9 device_id;
    ASSERT_DIRECTX(d3d->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &device_id));
    log->print(boost::str(boost::format("Graphical device: %s. Vertex shader version: %d.%d. "
                                        "Pixel shader version: %d.%d")
                              % device_id.Description
                              % ((device_caps.VertexShaderVersion & 0xff00) >> 8)
                              % (device_caps.VertexShaderVersion & 0xff)
                              % ((device_caps.PixelShaderVersion & 0xff00) >> 8)
                              % (device_caps.PixelShaderVersion & 0xff)));
    
    // Checking if device supports compressed textures
    D3DDEVICE_CREATION_PARAMETERS dp;
    device->GetCreationParameters(&dp);
    HRESULT hr = d3d->CheckDeviceFormat(dp.AdapterOrdinal, dp.DeviceType, D3DFMT_X8R8G8B8,
                                        0, D3DRTYPE_TEXTURE,
                                        (D3DFORMAT)TextureManagerInstance::DXT_METHOD);
    if (FAILED(hr))
    {
        log->warning("Graphical device doesn't support DXT3 compression.");
        TextureManagerInstance::DXT_METHOD = D3DFMT_A8R8G8B8;
        TextureManagerInstance::DXT_METHOD_height_shift = 0;
    }
    
    // Checking if render-to-texture is available
    if (conf->get<bool>("enable_render_to_texture"))
    {
        HRESULT hr = d3d->CheckDeviceFormat(dp.AdapterOrdinal, dp.DeviceType, D3DFMT_X8R8G8B8,
                                            D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, D3DRTYPE_TEXTURE,
                                            D3DFMT_A8R8G8B8);
        if (FAILED(hr))
            log->warning("Graphical device doesn't support rendering to texture.");
        else
        {
            rtt_enabled = true;
            log->print("Rendering to texture enabled.");
            
            // Obtaining default render target
            LPDIRECT3DSURFACE9 rt;
            ASSERT_DIRECTX(device->GetRenderTarget(0, &rt));
            default_render_target.push_back(rt);
        }
    }
    
    // Checking for hardware YUV conversion support
    if (!(device_caps.DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES) ||
        FAILED(d3d->CheckDeviceFormatConversion(dp.AdapterOrdinal, dp.DeviceType, D3DFMT_YUY2,
                                                D3DFMT_A8R8G8B8)) ||
        FAILED(d3d->CheckDeviceFormat(dp.AdapterOrdinal, dp.DeviceType, D3DFMT_X8R8G8B8, 0,
                                      D3DRTYPE_TEXTURE, D3DFMT_YUY2)) ||
        FAILED(d3d->CheckDeviceFormat(dp.AdapterOrdinal, dp.DeviceType, D3DFMT_X8R8G8B8,
                                      D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8)))
        log->warning("Graphical device doesn't support YUV conversion.");
    else
        hardware_yuv_enabled = true;
    
    if (!(device_caps.Caps2 & D3DCAPS2_DYNAMICTEXTURES))
        log->warning("Graphical device doesn't support dynamic textures");
    
    // Creating managers
    texture_manager.create();
    texture_manager->init(device);
    vertex_manager.create();
    sequence_manager.create();

    // Creating text renderer
    ASSERT_DIRECTX(D3DXCreateFont(device, 15, 0, 0, 0, 0, 0, 
                                  0, 0, 0, "Courier", &text_drawer));
    // Creating line renderer
    ASSERT_DIRECTX(D3DXCreateLine(device, &line_drawer));
    
    // Creating matrix stack
    ASSERT_DIRECTX(D3DXCreateMatrixStack(0, &matrix_stack));
    
    // Direct3D miscellaneous initializations
    zoom = 1.0f;
    setup_renderstate();
}

// Destructor
Direct3DInstance::~Direct3DInstance()
{
#define SAFE_RELEASE(res) { if (res) res->Release(); }

    SAFE_RELEASE(text_drawer);
    SAFE_RELEASE(line_drawer);
    // Releasing Direct3D resources
    sequence_manager.~sequence_manager();
    vertex_manager.~vertex_manager();
    texture_manager.~texture_manager();
    
    // Releasing Direct3D instances
    for (std::vector<LPDIRECT3DSURFACE9>::iterator i = default_render_target.begin();
         default_render_target.end() != i; ++i)
        SAFE_RELEASE((*i));
    SAFE_RELEASE(matrix_stack);
    SAFE_RELEASE(device);
    SAFE_RELEASE(d3d);
#undef SAFE_RELEASE
}

// Updating graphics system
void Direct3DInstance::update( DWORD dt )
{
    texture_manager->update(dt);
    vertex_manager->update(dt);
}

// Clear screen
void Direct3DInstance::clear( D3DCOLOR color )
{
    ASSERT(device);
    ASSERT_DIRECTX(device->Clear(NULL, NULL, D3DCLEAR_TARGET, color, 1.0f, 0));
}

// Saving screenshot
void Direct3DInstance::save_screenshot( char * filename, int width, int height )
{
    if(this->need_save_screenshot)
        return;

    this->need_save_screenshot = true;
    screenshot_name = filename;
    screenshot_width = width;
    screenshot_height = height;
}

// Present all geometry and flip buffers 
void Direct3DInstance::present()
{
    setup_matrices();

    if (need_save_screenshot)
    {
        Direct3D d3d;
        LPDIRECT3DSURFACE9 small_surface, old_surface;
        ASSERT_DIRECTX(d3d->device->CreateRenderTarget(screenshot_width, screenshot_height, 
                                                       D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, 
                                                       FALSE, &small_surface, NULL));
        ASSERT_DIRECTX(d3d->device->GetRenderTarget(0, &old_surface));
        ASSERT_DIRECTX(d3d->device->SetRenderTarget(0, small_surface));

        ASSERT_DIRECTX(device->BeginScene());
        TRY(sequence_manager->draw_queue());
        ASSERT_DIRECTX(device->EndScene());

        ASSERT_DIRECTX(d3d->device->SetRenderTarget(0, old_surface));
        ASSERT_DIRECTX(D3DXSaveSurfaceToFile(screenshot_name.c_str(), D3DXIFF_PNG, small_surface, NULL, NULL));
        
        small_surface->Release();
        old_surface->Release();
        
        need_save_screenshot = false;
    }
    
    ASSERT_DIRECTX(device->BeginScene());
    TRY(sequence_manager->flush());
    ASSERT_DIRECTX(device->EndScene());
    
    // Presenting scene
    HRESULT result = device->Present(NULL, NULL, NULL, NULL);
    if (D3DERR_DEVICELOST != result)
    {
        ASSERT_DIRECTX(result);
    }
    else
        is_device_lost = true;
}

// Direct3D matrix setup
void Direct3DInstance::setup_matrices()
{
    D3DXMatrixTranslation(&matrix_view, 0, 0, 5);
    
    const float w = 1024 / zoom, h = 768 / zoom;

    D3DXMatrixOrthoOffCenterLH(&matrix_projection, 0, w, h, 0, 
                               float(ZNear), float(ZFar));
    ASSERT_DIRECTX(device->SetTransform(D3DTS_VIEW, &matrix_view));
    ASSERT_DIRECTX(device->SetTransform(D3DTS_PROJECTION, &matrix_projection));
}

// Setup Direct3D rendering state
void Direct3DInstance::setup_renderstate()
{
    // Turning off z-testing (we will render objects in correct order)
    ASSERT_DIRECTX(device->SetRenderState(D3DRS_ZENABLE, FALSE));
    
    // Enabling culling (counter-clockwise polygons will be culled)
    ASSERT_DIRECTX(device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
    
    // Alpha testing for transparent backgrounds
    ASSERT_DIRECTX(device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE)); 
    ASSERT_DIRECTX(device->SetRenderState(D3DRS_ALPHAREF, 1));
    ASSERT_DIRECTX(device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL));
    
    // Enabling blending texture and material color
    ASSERT_DIRECTX(device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE));
    
    // Setting texture filters
    ASSERT_DIRECTX(device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT));
    ASSERT_DIRECTX(device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT));
    
    // Texture addressing mode
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    
    // Enabling alpha-blending
    ASSERT_DIRECTX(device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
    ASSERT_DIRECTX(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
    ASSERT_DIRECTX(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA)); 

    // Setting up lighting but not enabling it (we'll turn it off when
    // we really need it)
    ASSERT_DIRECTX(device->SetRenderState(D3DRS_LIGHTING, FALSE));
    
    // Default light
    D3DLIGHT9 light;
    ZeroMemory(&light, sizeof(light));
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r = 1.0f;
    light.Diffuse.g = 1.0f;
    light.Diffuse.b = 1.0f;
    light.Diffuse.a = 1.0f;

    light.Specular = light.Diffuse;
    light.Ambient = light.Diffuse;

    light.Direction.x = 0.0f;
    light.Direction.y = 0.0f;
    light.Direction.z = 1.0f;
    light.Range = ZFar;
    
    ASSERT_DIRECTX(device->SetLight(0, &light));
    ASSERT_DIRECTX(device->LightEnable(0, TRUE));
    ASSERT_DIRECTX(device->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, 
                                          D3DMCS_MATERIAL));

    // Default material
    D3DMATERIAL9 mtrl;
    ZeroMemory(&mtrl, sizeof(mtrl));
    D3DCOLORVALUE color = {1, 1, 1, 1};
    mtrl.Diffuse = mtrl.Ambient = mtrl.Specular = color;
    ASSERT_DIRECTX(device->SetMaterial(&mtrl));
    
    // Disabling vertex shader
    ASSERT_DIRECTX(device->SetVertexShader(NULL));
    
    // Setting transformation matrices
    setup_matrices();
}

// Get current transformation matrix
D3DXMATRIX Direct3DInstance::get_transform()
{
    return *matrix_stack->GetTop();
}

// Push transformation to stack
void Direct3DInstance::push_transform( const D3DXMATRIX & new_transform )
{
    matrix_stack->Push();
    matrix_stack->MultMatrixLocal(&new_transform);
}

// Pop transformation from stack
void Direct3DInstance::pop_transform()
{
    matrix_stack->Pop();
}

// Clear rendering queue
void Direct3DInstance::clear_render_queue()
{
    sequence_manager->clear_queue();
    matrix_stack->Release();
    ASSERT_DIRECTX(D3DXCreateMatrixStack(0, &matrix_stack));
}

// Check if device is lost and can be restored
bool Direct3DInstance::is_lost()
{
    if (!is_device_lost)
        return false;
    
    // Checking if device can be reset
    HRESULT hr = device->TestCooperativeLevel();
    switch (hr)
    {
    // Device is lost and cannot be restored
    case D3DERR_DEVICELOST:
        return true;
    
    // Device should be restored immediately
    case D3DERR_DEVICENOTRESET:
    
        // Releasing resources
        if (default_render_target.size() > 0)
        {
            ASSERT(default_render_target.size() == 1);
            default_render_target[0]->Release();
            default_render_target.clear();
        }
        
        texture_manager->unload_textures();
        text_drawer->OnLostDevice();
        line_drawer->OnLostDevice();
        VertexManagerInstance::VertexBufferInstance::force_update = true;
        // Resetting device
        ASSERT_DIRECTX(device->Reset(&present_params));
        
        // Restoring resources

        if (rtt_enabled)
        {
            // Obtaining default render target
            LPDIRECT3DSURFACE9 rt;
            ASSERT_DIRECTX(device->GetRenderTarget(0, &rt));
            default_render_target.push_back(rt);
        }
        
        text_drawer->OnResetDevice();
        line_drawer->OnResetDevice();
        
        setup_renderstate();
        is_device_lost = false;
        return true;
    }
    // Unknown error
    throw DirectXException("Device reset failed", hr);
}
