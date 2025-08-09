#pragma once
#include "Tanita2.h"
#include "Graphics.h"
#include "Log.h"
#include "FileManager.h"
#include <ddraw.h>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace ingame
{
class LargeAnimatedSequence;
class BeginRenderTargetSequence;
class CPUConversionVideoSequence;
class HWVideoSequence;
}

// Define this to enable texture collector debugging
// #define _DEBUG_TEXTURE_COLLECTOR

//! Texture memory manager
class Direct3DInstance::TextureManagerInstance
{
public:
    //! Smart reference to TextureInstance
    class TextureInstance
    {
    public:
        //! Destructor
        virtual ~TextureInstance() { unload(); }
        
        //! Get texture information
        /** \note dwReserved & 0xFF - original texture width (may be not power of 2)
                  dwReserved >> 16  - original texture height (may be not power of 2) */
        inline DDSURFACEDESC2 & get_description() { return texture_desc; }
            
        //! Bind texture to rendering pipeline
        void bind( int sampler_index = 0 );
        
        //! Sequence path (empty for dynamic textures)
        PATH path;
        
        //! Compression flag
        bool compressed;
        
        //! Get lowlevel texture handle
        LPDIRECT3DTEXTURE9 get_texture() { return texture; }

    protected:
        //! Construct from path to resource
        /** \param  path  path to texture */
        TextureInstance( PATH & path, bool compressed );
        //! Constructor
        inline TextureInstance() : texture(NULL), compressed(false) {}
    
        //! Texture memory manager info
        struct GCInfo
        {
            //! Approximate texture size
            DWORD size;
            //! Last usage time in frames (0 - used recently)
            DWORD age;
            //! Dynamic texture flag
            bool is_dynamic;

            //! Constructor
            inline GCInfo() : size(0), age(0), is_dynamic(false) {}
        } gc_info;
        //! File with texture data
        FileRef file;
        //! Texture surface description
        DDSURFACEDESC2 texture_desc;
        
        //! Create Direct3D texture
        /** Returns true if texture was loaded, false
          * if texture was already loaded before call */
        virtual bool load();
        //! Unload texture from video memory
        void unload();
        
        //! Direct3D texture pointer
        LPDIRECT3DTEXTURE9 texture;
        
        // Friend class
        friend class TextureManagerInstance;
        friend class ingame::BeginRenderTargetSequence;
    };
    //! Dynamic texture instance
    class DynamicTextureInstance: public TextureInstance
    {
    protected:
        // Constructor
        DynamicTextureInstance( int width, int height, bool compressed );
        
        // Create Direct3D texture
        virtual bool load();

        // Width and height
        int width, height;

        // Friend class
        friend class TextureManagerInstance;
        friend class ingame::LargeAnimatedSequence;
        friend class ingame::CPUConversionVideoSequence;
    };
    //! Texture for operating with surface
    class SurfaceTextureInstance: public TextureInstance
    {
    public:
        //! Destructor
        virtual ~SurfaceTextureInstance() { unload(); }
    
    protected:
        // Constructor
        SurfaceTextureInstance( int width, int height, D3DFORMAT format, 
                                bool render_target = false );
        // Create texture
        virtual bool load();
        // Release texture memory
        void unload();
        
        // Width and height
        int width, height;
        // Format
        D3DFORMAT format;
        // Is a render target surface
        bool render_target;
        
        // Texture surface
        LPDIRECT3DSURFACE9 surface;

        // Friend class
        friend class TextureManagerInstance;
        friend class ingame::HWVideoSequence;
    };
    //! Render target texture
    class RenderTargetTextureInstance: public TextureInstance
    {
    protected:
        // Constructor
        RenderTargetTextureInstance();
        
        // Create Direct3D texture
        virtual bool load();

        // Width and height
        int width, height;

        // Friend class
        friend class TextureManagerInstance;
        friend class ingame::BeginRenderTargetSequence;
    };
    typedef boost::shared_ptr<TextureInstance> TextureRef;
    
    //! Compression method used for compression textures
    static int DXT_METHOD;
    //! Height divider (for compression method)
    static int DXT_METHOD_height_shift;

    //! Texture manager initialization
    void init( LPDIRECT3DDEVICE9 device );
    //! Texture memory cleanup
    inline ~TextureManagerInstance() {};
    
    //! Updating textures
    void update( DWORD dt );
    
    //! Texture loading
    TextureRef load( PATH & path, bool compressed );
    
    //! Create texture for dynamic use
    TextureRef create_dynamic( int width, int height,
                               DynamicTextureInstance ** texture_ptr,
                               bool compressed );
    
    //! Create render target texture
    TextureRef create_render_target();
    
    //! Create texture with surface
    TextureRef create_surface_texture( int width, int height, D3DFORMAT format,
                                       SurfaceTextureInstance ** texture_ptr,
                                       bool render_target = false );
                               
    //! Releasing all texture memory for device restoring
    void unload_textures();
    
protected:
    //! Collect unused textures to free memory
    /** Called when texture creation fails with E_OUTOFMEMORY or
      * when we need to free more RAM. In the last case flag will
      * be set to true
      * @param[in]  low_sysmem_hint   false if cleanup triggered by low video 
                                      memory warning, true cleanup is needed to 
                                      free system memory */
    void collect( bool low_sysmem_hint = false );

    //! List of loaded textures
    std::vector<TextureRef> textures;
    typedef std::vector<TextureRef>::iterator TextureIter;
    
    //! Available texture memory
    static unsigned int available_mem;
    //! Utilized texture memory
    static unsigned int utilized_mem;
    
    //! Number of percents of video memory to free during collection
    static unsigned int percents_to_free;
    
    // Friends
    friend class TextureInstance;
    friend class FileManagerInstance;
};

//! Reference to texture type
typedef Direct3DInstance::TextureManagerInstance::TextureRef TextureRef;
