#include <stdafx.h>
#include "VertexManager.h"
#include "Log.h"
  
#define D3D_VM Direct3DInstance::VertexManagerInstance

// Force stream source update
bool D3D_VM::VertexBufferInstance::force_update = false;

// Vertex format
struct VertexFormat
{
    //! Position
    float x, y, z;
    //! Texture coordinates
    float u, v;

    //! Constructor
    VertexFormat( float x, float y, float u, float v )
        : x(x), y(y), z(0), u(u), v(v) {}
    //! Flexible vertex format description
    static const DWORD FVF = D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0);
};

// Vertex buffer creation
D3D_VM::VertexBufferRef D3D_VM::create( D3D_VM::VertexBufferData & data )
{
    // Checking if vertex buffer with same parameters was created previously
    static VertexBufferData prev_data(0, 0);
    static int prev_index;
    if (data == prev_data)
    {
        ASSERT(!vertex_buffers.empty());
        return VertexBufferRef(new VertexBufferInstance(*vertex_buffers.back(), prev_index));
    }
    // Creating vertex buffer if there are none or there is no space
    if (vertex_buffers.empty() || vertex_buffers.back()->free_space < 4)
        vertex_buffers.push_back(VertexBufferRef(new VertexBufferInstance(data)));
    
    VertexBufferRef & buffer = vertex_buffers.back();
    float width = float(data.width), height = float(data.height);
    
    // Sequence sprite vertices
    VertexFormat vertices[] =
    {
        VertexFormat(0,     0,         0.0f, 0.0f),
        VertexFormat(width, 0,         1.0f, 0.0f),
        VertexFormat(0,     height,    0.0f, 1.0f),
        VertexFormat(width, height,    1.0f, 1.0f),
    };
    
    // Adding sequence sprite to vertex buffer
    void * ptr;
    ASSERT_DIRECTX(buffer->vertex_buffer->Lock(
        (VertexBufferInstance::buffer_size - buffer->free_space) * sizeof(VertexFormat),
        sizeof(vertices), (void **)&ptr, 0));
    MoveMemory(ptr, vertices, sizeof(vertices));
    ASSERT_DIRECTX(buffer->vertex_buffer->Unlock());
    int start_index = VertexBufferInstance::buffer_size - buffer->free_space;
    prev_index = start_index;
    prev_data = data;
    buffer->free_space -= 4;

    // \todo: add vertex buffer caching support
    return VertexBufferRef(new VertexBufferInstance(*buffer, start_index));
}

// Construct from vertex buffer data
D3D_VM::VertexBufferInstance::VertexBufferInstance( D3D_VM::VertexBufferData & data )
    : free_space(buffer_size), start_index(0), vertex_buffer(NULL)
{
    Direct3D d3d;
    ASSERT_DIRECTX(d3d->device->CreateVertexBuffer(
        VertexBufferInstance::buffer_size * sizeof(VertexFormat),
        D3DUSAGE_WRITEONLY, VertexFormat::FVF, D3DPOOL_MANAGED, &vertex_buffer, NULL));
    Log log;
    log->debug("Vertex buffer created");
}

// Render from vertex buffer
void D3D_VM::VertexBufferInstance::render()
{
    Direct3D d3d;
    static LPDIRECT3DVERTEXBUFFER9 previous_vb;
    // Setting vertex buffer as stream source
    if (previous_vb != vertex_buffer || force_update)
    {
        force_update = false;
        previous_vb = vertex_buffer;
        ASSERT_DIRECTX(d3d->device->SetVertexShader(NULL));
        ASSERT_DIRECTX(d3d->device->SetFVF(VertexFormat::FVF));
        ASSERT_DIRECTX(d3d->device->SetStreamSource(0, vertex_buffer,
                       0, sizeof(VertexFormat)));
    }
    ASSERT_DIRECTX(d3d->device->DrawPrimitive(D3DPT_TRIANGLESTRIP, start_index, 2));    
}

#undef D3D_VM
