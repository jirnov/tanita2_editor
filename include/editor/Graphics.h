#pragma once
#include "Tanita2.h"
#include "Templates.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr9.h>

//! Rendering manager
class Direct3DInstance
{
public:
    //! Initializes Direct3D API and rendering infrastructure
    Direct3DInstance() throw(DirectXException_);
    //! Direct3D cleanup
    ~Direct3DInstance();
    
    //! Updating graphics system
    void update( DWORD dt );
    
    //! Clear screen
    /** @param  color  color to clear */
    void clear( D3DCOLOR color );
    //! Present all geometry and flip buffers 
    void present();

    //! Get projection matrix
    inline const D3DXMATRIX & get_projection_matrix() const
        { return matrix_projection; }
    //! Get view matrix
    inline const D3DXMATRIX & get_view_matrix() const
        { return matrix_view; }

    //! Multiply current world matrix and push to stack
    /** @param  new_transform  transformation to apply */
    void push_transform( const D3DXMATRIX & new_transform );
    //! Pop transformation matrix from stack
    void pop_transform();
    //! Get current transformation matrix (matrix stack top)
    D3DXMATRIX get_transform();

	// Flag indicating that we need to save screenshot
	void save_screenshot( char * filename, int width, int height );
    
    // Texture memory manager
    class TextureManagerInstance;
    //! TextureManagerInstance singleton definition
    typedef Singleton<TextureManagerInstance> TextureManager;

    // Vertex buffer manager
    class VertexManagerInstance;
    //! VertexManagerInstance singleton definition
    typedef Singleton<VertexManagerInstance> VertexManager;
    
    // Animation sequence rendering manager
    class SequenceManagerInstanceBase;
    class SequenceManagerInstance;
    // SequenceManagerInstance singleton definition
    typedef Singleton<SequenceManagerInstance> SequenceManager;
    
    // Effects manager
    class EffectsManagerInstance;
    //! TextureManagerInstance singleton definition
    typedef Singleton<EffectsManagerInstance> EffectsManager;
    
    //! Text renderer
    LPD3DXFONT text_drawer;
    //! Line renderer
    LPD3DXLINE line_drawer;
    //! Zoom factor
    float zoom;
  
    //! View and projection matrix setup
    void setup_matrices();

    //! Device capabilities
    D3DCAPS9 device_caps;
    //! Render-to-texture support flag
    bool rtt_enabled;
    //! Hardware YUV conversion support flag
    bool hardware_yuv_enabled; 

	//! Flag indicating that we need to save screenshot
	static bool need_save_screenshot;
    
    // Default render target
    std::vector<LPDIRECT3DSURFACE9> default_render_target;

protected:

    //! Direct3D object
    LPDIRECT3D9 d3d;
    //! Current present parameters
    D3DPRESENT_PARAMETERS present_params;
    //! Texture manager
    SINGLETON(TextureManager) texture_manager;
    //! Vertex buffer manager
    SINGLETON(VertexManager) vertex_manager;
    //! Animation sequence rendering manager
    SINGLETON(SequenceManager) sequence_manager;
    
    //! Render state setup
    void setup_renderstate();
    
    //! Near and far clipping planes
    static const int ZNear = 1, ZFar = 1000;
    //! Matrix stack
    LPD3DXMATRIXSTACK matrix_stack;

	//! Values for saving  screenshot
	std::string screenshot_name;
	int screenshot_width;
	int screenshot_height;
    
    //! Projection matrix
    D3DXMATRIX matrix_projection;
    //! View matrix
    D3DXMATRIX matrix_view;
    
    //! Flag indicating that device was lost
    bool is_device_lost;
    
public:
    //! Check if device is lost and can be restored
    bool is_lost();

    //! Device object
    LPDIRECT3DDEVICE9 device;
    
    //! Clear rendering queue
    /** Used in on_script_reload function of ApplicationInstance */
    void clear_render_queue();
};
//! Direct3D singleton definition
typedef Singleton<Direct3DInstance> Direct3D;

//! Texture manager
typedef Direct3DInstance::TextureManager TextureManager;
//! Vertex buffer manager
typedef Direct3DInstance::VertexManager VertexManager;
//! Animation sequence rendering manager
typedef Direct3DInstance::SequenceManager SequenceManager;
//! Effects rendering manager
typedef Direct3DInstance::EffectsManager EffectsManager;
