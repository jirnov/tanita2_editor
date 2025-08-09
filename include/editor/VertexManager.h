#ifndef _EDITOR_VERTEXMANAGER_H_
#define _EDITOR_VERTEXMANAGER_H_

#include "Tanita2.h"
#include "Graphics.h"

//! Vertex buffer manager
class Direct3DInstance::VertexManagerInstance
{
public:
    //! Vertex buffer description type
    struct VertexBufferData
    {
    public:
        int width;
        int height;

        //! Constructor
        /** \param  w  sprite width
          * \param  h  sprite height */
        inline VertexBufferData( int w, int h )
            : width(w), height(h) {}
            
    protected:
        //! Constructor
        inline VertexBufferData() : width(0), height(0) {}
        //! Comparison operator
        /** This operator is used to compare creation parameters
          * for few vertex buffers to merge buffers with same parameters 
          * \param  d  vertex buffer data for comparison */
        inline bool operator ==( const VertexBufferData & d )
            { return width == d.width && height == d.height; }
            
        // Friend
        friend class VertexManagerInstance;
    };

    //! Reference to vertex buffer
    class VertexBufferInstance
    {
    public:
        //! Destructor
        inline ~VertexBufferInstance() { vertex_buffer->Release(); }
        
        //! Render contents of vertex buffer
        void render();

    protected:
        //! Construct from vertex buffer data
        /** \param  data  vertex buffer description data */
        VertexBufferInstance( VertexBufferData & data );
        //! Construct from already existing vertex buffer with new start_index
        /** Vertex buffer may be shared between multiple reference. In this case
          * each reference holds pointer to vertex buffer and unique offset
          * to vertex data.
          * \param  v            existing vertex buffer
          * \param  start_index  offset to vertex data hold by this reference */
        inline VertexBufferInstance( const VertexBufferInstance & v, int start_index )
            : start_index(start_index), free_space(v.free_space),
              vertex_buffer(v.vertex_buffer) { vertex_buffer->AddRef(); }
    
        //! Start index in DrawPrimitive call
        UINT start_index; 
        //! Number of vertexes we could allocate in this buffer
        int free_space;
        //! Size of vertex buffer (in vertexes)
        static const int buffer_size = 2000;
        
        //! Pointer to Direct3D vertex buffer
        LPDIRECT3DVERTEXBUFFER9 vertex_buffer;
        
        //! Force stream source update
        static bool force_update;
        
        // Friend
        friend class VertexManagerInstance;
        friend class Direct3DInstance;
    };
    typedef boost::shared_ptr<VertexBufferInstance> VertexBufferRef;

    //! Vertex manager initialization
    VertexManagerInstance() {}
    //! Vertex buffer cleanup
    ~VertexManagerInstance() {}
    
    //! Updating vertex buffers
    inline void update( DWORD dt ) {}

    //! Allocate vertex buffer
    /** @param  data  information for vertex buffer allocation
    * @return reference to vertex buffer */
    VertexBufferRef create( VertexBufferData & data );

protected:
    //! List of vertex buffers
    std::vector<VertexBufferRef> vertex_buffers;
    typedef std::vector<VertexBufferRef>::iterator BufferIter;
    
    //! Create DirectX vertex buffer instance
    /** \param  data  vertex buffer description data */
    LPDIRECT3DVERTEXBUFFER9 create_vertex_buffer( VertexBufferData & data );
};

//! Vertex buffer description type definition
typedef Direct3DInstance::VertexManagerInstance::VertexBufferData VertexBufferData;
//! Reference to vertex buffer type
typedef Direct3DInstance::VertexManagerInstance::VertexBufferRef VertexBufferRef;

#endif // _EDITOR_VERTEXMANAGER_H_

