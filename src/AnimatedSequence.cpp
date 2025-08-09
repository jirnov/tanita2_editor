#include "stdafx.h"
#include "Sequence.h"
#include "Graphics.h"
#include "GameObject.h"

using namespace ingame;

// Constructor
AnimatedSequenceBase::AnimatedSequenceBase( std::vector<int> & frame_indices )
    : time_left(0)
{
    ingameASSERT(frame_indices.size() > 0 && "Animation has no frames");
    frame_count = frame_indices.size();
    current_frame_number = 0;
    is_playing = true;
}

// Updating
bool AnimatedSequenceBase::on_adding_to_queue( float dt )
{
    is_over = false;
    int df = (flags & reversed) ? -1 : 1;
    
    if (is_playing)
    {
        time_left -= dt;
        while (time_left < 0)
        {
            if (!(flags & reversed)) // Normal animation
            {
                current_frame_number++;
                if (current_frame_number >= frame_count)
                {
                    is_over = true;
                    
                    // Stopping animation or letting it play from start
                    if (flags & looped)
                        current_frame_number = 0;
                    else
                    {
                        current_frame_number = frame_count-1;
                        is_playing = false;
                        time_left = 0;
                        break;
                    }
                }
            }
            else
            {
                current_frame_number--;
                if (current_frame_number < 0)
                {
                    is_over = true;
                
                    // Stopping animation or letting it play from start
                    if (flags & looped)
                        current_frame_number = frame_count-1;
                    else
                    {
                        current_frame_number = 0;
                        is_playing = false;
                        time_left = 0;
                        break;
                    }
                }
            }
            time_left += 1.0f / fps;
        }
    }
    return SequenceBase::on_adding_to_queue(dt);
}

// Creating sequence for given path and texture count
AnimatedSequence::AnimatedSequence( PATH & path, std::vector<int> & frame_indices, bool compressed )
    : AnimatedSequenceBase(frame_indices)
{
    // Loading texture
    TextureManager tm;

    for (int j = 0; j < (int)frame_indices.size(); ++j)
    {
        FrameInfo f;
		int i = frame_indices[j];
		PATH p(path.get_directory(), i, PATH::PNG);
        TRY(f.texture = tm->load(p, compressed ? FM_HINT_COMPRESSED_TEXTURE : FM_HINT_NONE));

        DDSURFACEDESC2 desc = f.texture->get_description();

        // Creating vertex buffer
        VertexManager vm;
        VertexBufferData data(desc.dwWidth, desc.dwHeight);
        TRY(f.buffer = vm->create(data));

        bounding_box.x = float(desc.dwReserved & 0xFFFF);
        bounding_box.y = float(desc.dwReserved >> 16);
        frames.push_back(f);
    }
}

// Sequence rendering
void AnimatedSequence::render()
{
    Direct3D d3d;
    d3d->device->SetTransform(D3DTS_WORLD, &transformation);
    FrameInfo & f = frames[current_frame_number];
    TRY(f.texture->bind());
    TRY(f.buffer->render());
}

// Creating sequence for given path and texture count
LargeAnimatedSequence::LargeAnimatedSequence( PATH & path, std::vector<int> & frame_indices, bool compressed )
    : AnimatedSequenceBase(frame_indices), lowlevel_texture(NULL), loaded_frame(-1)
{
    FileManager fm;
    for (int j = 0; j < (int)frame_indices.size(); ++j)
    {
        FileRef f;
		int i = frame_indices[j];
		PATH p(path.get_directory(), i, PATH::PNG);
        TRY(f = fm->load(p, compressed ? FM_HINT_COMPRESSED_TEXTURE : FM_HINT_NONE));
        width = *((DWORD *)f->get_contents() + 4);
        height = *((DWORD *)f->get_contents() + 3);

        char * const dds = f->get_contents();
        bounding_box.x = float(((DDSURFACEDESC2 *)(dds + 4))->dwReserved & 0xFFFF);
        bounding_box.y = float(((DDSURFACEDESC2 *)(dds + 4))->dwReserved >> 16);

        frames.push_back(f);
    }
	
    // Loading texture
    TextureManager tm;
    TRY(texture = tm->create_dynamic(width, height, &lowlevel_texture, true));
    // Creating vertex buffer
    VertexManager vm;
    VertexBufferData data(width, height);
    TRY(buffer = vm->create(data));
}

//! Updating
bool LargeAnimatedSequence::on_adding_to_queue( float dt )
{
    // Checking if parent's update function has altered frame number
    bool result = AnimatedSequenceBase::on_adding_to_queue(dt);
    if (current_frame_number != loaded_frame)
    {
        loaded_frame = current_frame_number;
        fill_texture();
    }
    return result;
}

// Filling texture with current frame
void LargeAnimatedSequence::fill_texture()
{
    D3DLOCKED_RECT lr;
    lowlevel_texture->load();
        
    LPDIRECT3DTEXTURE9 tex = lowlevel_texture->texture;
    
    Direct3D d3d;
    static DWORD lock_flag =
        (d3d->device_caps.Caps2 & D3DCAPS2_DYNAMICTEXTURES) ? D3DLOCK_DISCARD : 0;
    
    ASSERT_DIRECTX(tex->LockRect(0, &lr, NULL, lock_flag));
    FileRef & f = frames[current_frame_number];
    BYTE * ptr = (BYTE *)f->get_contents() + 128;
    
    int h = texture->compressed ?
                height >> Direct3DInstance::TextureManagerInstance::DXT_METHOD_height_shift :
                height;
    for (int y = 0; y < h; ++y)
        memcpy((BYTE *)lr.pBits + y * lr.Pitch, ptr + y * lr.Pitch, lr.Pitch);
    ASSERT_DIRECTX(tex->UnlockRect(0));
}

// Sequence rendering
void LargeAnimatedSequence::render()
{
    Direct3D d3d;
    d3d->device->SetTransform(D3DTS_WORLD, &transformation);

    if (lowlevel_texture->load()) // load() returns true if texture was really loaded during call
        fill_texture();
    TRY(texture->bind());
    TRY(buffer->render());
}
