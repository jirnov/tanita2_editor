#include "stdafx.h"
#include "TextureManager.h"
#include "Application.h"

#define D3D_TM Direct3DInstance::TextureManagerInstance

// Available texture memory
unsigned int D3D_TM::available_mem;
// Utilized texture memory
unsigned int D3D_TM::utilized_mem;
// Number of percents of video memory to free during collection
unsigned int D3D_TM::percents_to_free;
// Compression method used for compression textures
int D3D_TM::DXT_METHOD = D3DFMT_DXT3;
//! Height divider (for compression method)
int D3D_TM::DXT_METHOD_height_shift = 2;

// Constructor
D3D_TM::TextureInstance::TextureInstance( PATH & path, bool compressed )
    : texture(NULL), compressed(compressed)
{
    ZeroMemory(&texture_desc, sizeof(DDSURFACEDESC2));
    
    // Loading file
    FileManager fm;
    TRY(file = fm->load(path, compressed ? FileManagerInstance::HINT_COMPRESSED_TEXTURE : 
                                           FileManagerInstance::HINT_NONE));
    
    // Obtaining texture info
    char * const dds = file->get_contents();
    // DDS signature
    ASSERT(*(DWORD *)dds == (('D' << 0) | ('D' << 8) | ('S' << 16) | (' ' << 24)));
    CopyMemory(&texture_desc, dds + 4, sizeof(texture_desc));
    ASSERT(0 != (texture_desc.dwReserved & 0xFFFF) && 0 != (texture_desc.dwReserved >> 16));
    gc_info.size = texture_desc.dwWidth * texture_desc.dwHeight * 4 / 1024;
    if (compressed) 
        gc_info.size /= 4;
}

// Macro for creating texture with E_OUTOFMEMORY handling
#define CREATE_TEXTURE(create_func_call)                          \
    {                                                             \
        HRESULT hr = create_func_call;                            \
        if (FAILED(hr))                                           \
        {                                                         \
            if (D3DERR_OUTOFVIDEOMEMORY == hr)                    \
            {                                                     \
                Singleton<TextureManagerInstance> tm;             \
                UINT old_ptf = percents_to_free;                  \
                percents_to_free = 100;                           \
                tm->collect();                                    \
                percents_to_free = old_ptf;                       \
                hr = create_func_call;                            \
            }                                                     \
            if (FAILED(hr))                                       \
                throw DirectXException(#create_func_call, hr);    \
        }                                                         \
    }

// Load texture to Direct3D
bool D3D_TM::TextureInstance::load()
{
    if (texture) return false;
    Direct3D d3d;
    
    // Какой идиот из M$ сделал первой высоту, а не ширину картинки???
    DWORD w = *((DWORD *)file->get_contents() + 4);
    DWORD h = *((DWORD *)file->get_contents() + 3);
    CREATE_TEXTURE(D3DXCreateTextureFromFileInMemoryEx(d3d->device, 
                       file->get_contents(), file->get_size(), w, 
                       h, 1, 0, compressed ? (D3DFORMAT)D3D_TM::DXT_METHOD : D3DFMT_A8R8G8B8,
                       D3DPOOL_DEFAULT, D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0,
                       NULL, NULL, &texture));
    TextureManagerInstance::utilized_mem += gc_info.size;
    return true;
}

// Unloading texture from video memory
void D3D_TM::TextureInstance::unload()
{
    if (!texture) return;
    texture->Release();
    texture = NULL;
    
    TextureManagerInstance::utilized_mem -= gc_info.size;
}

// Bind texture to Direct3D
void D3D_TM::TextureInstance::bind( int sampler_index )
{
    load();
    gc_info.age = 0;
    Direct3D d3d;
    ASSERT_DIRECTX(d3d->device->SetTexture(sampler_index, texture));
}

// Texture loading
D3D_TM::TextureRef D3D_TM::load( PATH & path, bool compressed )
{
    Config cfg;
    if (!cfg->get<bool>("disable_file_cache"))
    {

    // Checking if texture was loaded before
    for (TextureIter i = textures.begin(); textures.end() != i; ++i)
        if ((*i)->path == path)
            return *i;

    }

    ApplicationInstance::pause();
    textures.push_back(TextureRef(new TextureInstance(path, compressed)));
    TextureRef & t = textures.back();
    t->path = path;
    ApplicationInstance::resume();
    return t;
}

// Constructor
D3D_TM::DynamicTextureInstance::DynamicTextureInstance( int width, int height, bool compressed )
    : width(width), height(height)
{
    this->compressed = compressed;
    
    ZeroMemory(&texture_desc, sizeof(DDSURFACEDESC2));
    texture_desc.dwWidth = width;
    texture_desc.dwHeight = height;
    gc_info.size = texture_desc.dwWidth * texture_desc.dwHeight * 4 / 1024;
    if (compressed)
        gc_info.size /= 4;
    gc_info.is_dynamic = true;
}

// Create Direct3D texture
bool D3D_TM::DynamicTextureInstance::load()
{
    if (texture) return false;
    Direct3D d3d;
    Log log;

    // Some Intel cards don't support dynamic textures, using managed instead
    static D3DPOOL pool = D3DPOOL_DEFAULT;
    static DWORD usage = (d3d->device_caps.Caps2 & D3DCAPS2_DYNAMICTEXTURES) ? 
                              // Device supports dynamic textures
                              D3DUSAGE_DYNAMIC :
                              // Device doesn't support: using managed pool
                              (pool = D3DPOOL_MANAGED, 0);
    CREATE_TEXTURE(d3d->device->CreateTexture(width, height, 1, usage,
                       compressed ? (D3DFORMAT)D3D_TM::DXT_METHOD : D3DFMT_A8R8G8B8, 
                       pool, &texture, NULL));
    // Filling with garbage for debug
    D3DLOCKED_RECT lr;
    ASSERT_DIRECTX(texture->LockRect(0, &lr, NULL, usage ? D3DLOCK_DISCARD : 0));
    int h = compressed ?
                height >> Direct3DInstance::TextureManagerInstance::DXT_METHOD_height_shift :
                height;
    for (int y = 0; y < h; ++y)
        memset((BYTE *)lr.pBits + y * lr.Pitch, 0xcc, lr.Pitch);
    ASSERT_DIRECTX(texture->UnlockRect(0));

    TextureManagerInstance::utilized_mem += gc_info.size;
    return true;
}

// Constructor
D3D_TM::SurfaceTextureInstance::SurfaceTextureInstance( int width, int height, 
                                                        D3DFORMAT format, bool render_target )
    : width(width), height(height), format(format), 
      surface(NULL), render_target(render_target)
{
    this->compressed = false;
    ZeroMemory(&texture_desc, sizeof(DDSURFACEDESC2));
    texture_desc.dwWidth = width;
    texture_desc.dwHeight = height;
    
    gc_info.size = texture_desc.dwWidth * texture_desc.dwHeight * 4 / 1024;
    gc_info.is_dynamic = false;
}

// Create texture
bool D3D_TM::SurfaceTextureInstance::load()
{
    if (texture) return false;
    
    Direct3D d3d;
    Log log;
    static DWORD usage = (d3d->device_caps.Caps2 & D3DCAPS2_DYNAMICTEXTURES) ? D3DUSAGE_DYNAMIC : 0;
    const DWORD real_usage = (render_target ? D3DUSAGE_RENDERTARGET : usage);
    CREATE_TEXTURE(d3d->device->CreateTexture(width, height, 1, real_usage, format, 
                                              D3DPOOL_DEFAULT, &texture, NULL));
    // Obtaining surface
    ASSERT_DIRECTX(texture->GetSurfaceLevel(0, &surface));

    // Updating gc info
    TextureManagerInstance::utilized_mem += gc_info.size;
    return true;
}

// Release resources
void D3D_TM::SurfaceTextureInstance::unload()
{
    if (surface)
    {
        surface->Release();
        surface = NULL;
    }
}

// Constructor
D3D_TM::RenderTargetTextureInstance::RenderTargetTextureInstance()
    : width(0), height(0)
{
    // Determining width and height
    Direct3D d3d;
    width = d3d->present_params.BackBufferWidth;
    height = d3d->present_params.BackBufferHeight;
    if ((d3d->device_caps.TextureCaps & D3DPTEXTURECAPS_POW2) &&
        !(d3d->device_caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL))
    {
        int w = 256;
        while (w < width)
            w <<= 1;
        width = height = w;
    }

    ZeroMemory(&texture_desc, sizeof(DDSURFACEDESC2));
    texture_desc.dwWidth = width;
    texture_desc.dwHeight = height;
    gc_info.size = texture_desc.dwWidth * texture_desc.dwHeight * 4 / 1024;
}

// Create render target texture
bool D3D_TM::RenderTargetTextureInstance::load()
{
    if (texture) return false;
    Direct3D d3d;
    Log log;
    CREATE_TEXTURE(d3d->device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET,
                                              D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL));
    TextureManagerInstance::utilized_mem += gc_info.size;
    return true;
}
#undef CREATE_TEXTURE

// Create texture for dynamic use
D3D_TM::TextureRef D3D_TM::create_dynamic( int width, int height,
                                           DynamicTextureInstance ** texture_ptr,
                                           bool compressed )
{
    ASSERT(NULL != texture_ptr);
    *texture_ptr = new DynamicTextureInstance(width, height, compressed);
    textures.push_back(TextureRef(*texture_ptr));
    return textures.back();
}

// Create render target texture
D3D_TM::TextureRef D3D_TM::create_render_target()
{
    textures.push_back(TextureRef(new RenderTargetTextureInstance()));
    return textures.back();
}

// Create texture with surface
D3D_TM::TextureRef D3D_TM::create_surface_texture( int width, int height, D3DFORMAT format,
                                                   SurfaceTextureInstance ** texture_ptr,
                                                   bool render_target )
{
    ASSERT(NULL != texture_ptr);
    *texture_ptr = new SurfaceTextureInstance(width, height, format, render_target);
    textures.push_back(TextureRef(*texture_ptr));
    return textures.back();
}

// Initialization
void D3D_TM::init( LPDIRECT3DDEVICE9 device )
{
    Config config;
    try { percents_to_free = (UINT)config->get<int>("vmem_hyst"); }
    catch (...) { percents_to_free = 20; }
    
    available_mem = 0;
    try { int vmem_limit = (UINT)config->get<int>("video_memory_limit");
          if (0 < vmem_limit) available_mem = vmem_limit; }
    catch (...) { percents_to_free = 20; }
    
    if (0 == available_mem)
        available_mem = device->GetAvailableTextureMem() / 1024;
    
    Log log;
    log->print(boost::str(boost::format("Available video memory: %dMb") % (available_mem / 1024)));
}

// Free unused texture memory
void D3D_TM::collect( bool low_sysmem_hint )
{
#ifndef _DEBUG_TEXTURE_COLLECTOR
    // Updating available memory count
    if (!low_sysmem_hint && utilized_mem < available_mem)
    {
        Log log;
        log->debug(boost::str(boost::format("Updating texture gc available memory size:"
                                            " %dMB instead of %dMB") % 
                                            (utilized_mem / 1024) % (available_mem / 1024)));
        available_mem = utilized_mem;
    }
#endif
    // Memory size to reach (maximum if cleaning system memory instead of video)
    UINT to_reach = low_sysmem_hint ? 0 : available_mem * (100 - percents_to_free) / 100;
    
    // Aggressively freeing textures from other scenes
    for (TextureIter i = textures.begin(); textures.end() > i; ++i)
    {
        if (!i->unique()) continue;
        
        (*i)->unload();
        i = textures.erase(i);
        
        if (utilized_mem < to_reach)
            return;
    }
    if (low_sysmem_hint) return; // We've done with cleaning unneeded system memory objects
    
    // Freeing textures that are not used currently (non-visible)
    for (TextureIter i = textures.begin(); textures.end() > i; ++i)
    {
        TextureInstance::GCInfo & info = (*i)->gc_info;
        if (info.age > 70 /* 15fps */)
            (*i)->unload();
        if (utilized_mem < to_reach) return;
    }
}

// Updating textures
void D3D_TM::update( DWORD dt )
{
    // Skipping if no actual update or if there are plenty of video mem
    if (0 == dt || utilized_mem * 100 / available_mem < 100 - percents_to_free) return;
    
    // Releasing textures that are not used in current scene
    for (TextureIter i = textures.begin(); textures.end() > i; ++i)
    {
        TextureInstance::GCInfo & info = (*i)->gc_info;
        if (i->unique() && info.age > 10000) // Ten seconds from last low memory warning
        {
            (*i)->unload();
            i = textures.erase(i);
            continue;
        }
        info.age += dt;
    }
}

// Releasing all texture memory for device restoring
void D3D_TM::unload_textures()
{
    for (TextureIter i = textures.begin(); textures.end() > i; ++i)
        (*i)->unload();
}

#undef D3D_TM
