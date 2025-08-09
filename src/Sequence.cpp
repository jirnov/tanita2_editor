#include "stdafx.h"
#include "Sequence.h"

using namespace ingame;

// Constructor
StaticSequence::StaticSequence( PATH & texture_path, bool compressed )
{
    // Loading texture
    TextureManager tm;
    TRY(texture = tm->load(texture_path, compressed ? FM_HINT_COMPRESSED_TEXTURE : FM_HINT_NONE));
    DDSURFACEDESC2 desc = texture->get_description();

    // Creating vertex buffer
    VertexManager vm;
    VertexBufferData data(desc.dwWidth, desc.dwHeight);
    TRY(vbuffer = vm->create(data));
    
    bounding_box.x = float(desc.dwReserved & 0xFFFF);
    bounding_box.y = float(desc.dwReserved >> 16);
}

// Sequence rendering
void StaticSequence::render()
{
    Direct3D d3d;
    d3d->device->SetTransform(D3DTS_WORLD, &transformation);

    TRY(texture->bind());
    TRY(vbuffer->render());
}

// Gizmo vertex format
struct GizmoVertexFormat
{
    // Position
    float x, y, z;
    // Color
    DWORD color;

    // Constructor
    GizmoVertexFormat( float x, float y, DWORD color )
        : x(x), y(y), z(0), color(color) {}
    // Flexible vertex format description
    static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
};

// Sequence rendering
void GizmoSequence::render()
{
    Direct3D d3d;
    float & w = bounding_box.x, & h = bounding_box.y;

    DWORD oldFVF;
    LPDIRECT3DVERTEXBUFFER9 oldStreamData;
    UINT oldOffsetInBytes, oldStride;
    ASSERT_DIRECTX(d3d->device->GetStreamSource(0, &oldStreamData, &oldOffsetInBytes, &oldStride));
    ASSERT_DIRECTX(d3d->device->GetFVF(&oldFVF));
    
    GizmoVertexFormat v[4] = {GizmoVertexFormat(0, 0, color),
                              GizmoVertexFormat(w, 0, color),
                              GizmoVertexFormat(0, h, color),
                              GizmoVertexFormat(w, h, color)};
    ASSERT_DIRECTX(d3d->device->SetFVF(GizmoVertexFormat::FVF));
    ASSERT_DIRECTX(d3d->device->SetTexture(0, NULL));
    d3d->device->SetTransform(D3DTS_WORLD, &transformation);
    ASSERT_DIRECTX(d3d->device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(GizmoVertexFormat)));
    
    ASSERT_DIRECTX(d3d->device->SetStreamSource(0, oldStreamData, oldOffsetInBytes, oldStride));
    ASSERT_DIRECTX(d3d->device->SetFVF(oldFVF));
    if (oldStreamData) oldStreamData->Release();
}

// Sequence rendering
void TextSequence::render()
{
    Direct3D d3d;
    D3DXVECTOR4 np;
    D3DXVec2Transform(&np, &position, &transformation);
    long px = long(np.x), py = long(np.y);
    RECT r = {px, py, px+1, py+1};
    d3d->text_drawer->DrawText(NULL, text.c_str(), -1, &r, DT_NOCLIP, color);
}
