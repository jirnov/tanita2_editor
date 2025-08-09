#include "stdafx.h"
#include "SoundManager.h"
#include "Sound.h"
#include "Graphics.h"

// Useful definitions
#define D3D_SM DirectSoundInstance::SoundManagerInstance
#define D3D_SMBASE DirectSoundInstance::SoundManagerInstanceBase

// Constructor
D3D_SMBASE::SoundBase::SoundBase()
    : position(0, 0), flags(0), is_playing(false), is_over(false),
      pan(0), volume(0), buffer(NULL), notification(0),
      notification_obj(NULL), priority(0), timestamp(0), 
      force_no_loop(false), is_old(false), buffer_size(0),
      is_static(false), force_stream_loop(false), is_music(false)
{
    ZeroMemory(&sound_desc, sizeof(sound_desc));
}

// Play sound
void D3D_SMBASE::SoundBase::play()
{
    if (is_playing || is_old) return;
    
    // Playing sound
    is_playing = true;
    ASSERT_DIRECTX(buffer->Play(0, priority, DSBPLAY_TERMINATEBY_PRIORITY | 
                                             DSBPLAY_TERMINATEBY_TIME |
                                             (is_buffer_looped() ? DSBPLAY_LOOPING : 0)));
}

// Stop (pause) sound
void D3D_SMBASE::SoundBase::stop()
{
    if (!is_playing) return;
    
    // Pausing sound
    is_playing = false;
    DirectSound ds;
    ASSERT_DIRECTX(buffer->Stop());
}

// Rewind sound to start
void D3D_SMBASE::SoundBase::rewind()
{
    // Rewind sound
    DirectSound ds;
    stop();
    ASSERT_DIRECTX(buffer->SetCurrentPosition(0));
}

#define TO_DS_VOLUME(v) (DSBVOLUME_MIN / 2 + (v) * 50)

// Set volume
void D3D_SMBASE::SoundBase::set_volume( int new_volume )
{
    buffer->SetVolume(TO_DS_VOLUME(new_volume));
}

// Set pan
void D3D_SMBASE::SoundBase::set_pan( int new_pan )
{
    buffer->SetPan(new_pan * 50);
}

#undef TO_DS_VOLUME

// Unloading sound
void D3D_SMBASE::SoundBase::unload()
{
    if (buffer) buffer->Release();
    if (notification) CloseHandle(notification);
    if (notification_obj) notification_obj->Release();
}

// Called when sound is to be added to render queue
bool D3D_SMBASE::SoundBase::on_adding_to_queue( float dt )
{
    Direct3D d3d;
    D3DXMATRIX m;
    D3DXMatrixTranslation(&m, position.x, position.y, 0.0f);
    transformation = m * d3d->get_transform();
    return true;
}

#undef D3D_SM
#undef D3D_SMBASE
