#include "stdafx.h"
#include "SoundManager.h"
#include "Sound.h"

// Useful definitions
#define D3D_SM DirectSoundInstance::SoundManagerInstance
#define D3D_SMBASE DirectSoundInstance::SoundManagerInstanceBase

// Creating sound from wave file
D3D_SMBASE::StaticSound::StaticSound( PATH & sound_path )
    : SoundBase(), path(sound_path), data_ptr(NULL), data_size(0)
{
    is_static = true;
    
    // Loading file
    FileManager fm;
    TRY(file = fm->load(sound_path));
    
    // Assuming that file was validated by file manager
    DWORD * ptr = (DWORD *)file->get_contents();
    sound_desc = *(WAVEFORMATEX *)(ptr + 5);
    sound_desc.cbSize = 0;
    
    // Getting data size and data pointer
    buffer_size = data_size = *(DWORD *)((BYTE *)ptr + *ptr);
    data_ptr = (BYTE *)ptr + *ptr + 4;
    
    // Creating secondary sound buffer
    DSBUFFERDESC descr;
    ZeroMemory(&descr, sizeof(DSBUFFERDESC));
    descr.dwSize = sizeof(DSBUFFERDESC);
    descr.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME |
                    DSBCAPS_LOCDEFER | DSBCAPS_CTRLPOSITIONNOTIFY;
    descr.dwBufferBytes = data_size;
    descr.guid3DAlgorithm = DS3DALG_DEFAULT;
    descr.lpwfxFormat = &sound_desc;
    DirectSound ds;
    ASSERT_DIRECTX(ds->dsound->CreateSoundBuffer(&descr, &buffer, NULL));
    
    // Creating notification object
    notification = CreateEvent(NULL, 0, 0, NULL);
    ASSERT_DIRECTX(buffer->QueryInterface(IID_IDirectSoundNotify8, (void **)&notification_obj));
    
    // Setting notification
    DSBPOSITIONNOTIFY pn;
    pn.dwOffset = data_size-1;
    pn.hEventNotify = notification;
    ASSERT_DIRECTX(notification_obj->SetNotificationPositions(1, &pn));
    
    // Filling sound buffer
    fill();
}

// Copy constructor
D3D_SMBASE::StaticSound::StaticSound( const StaticSound & sound )
    : SoundBase(sound), path(sound.path), file(sound.file),
      data_ptr(sound.data_ptr), data_size(sound.data_size)
{
    ASSERT(NULL != sound.buffer);
    is_static = true;
    buffer_size = data_size;

    // Duplicating buffer
    DirectSound ds;
    ASSERT_DIRECTX(ds->dsound->DuplicateSoundBuffer(sound.buffer, &buffer));
    // DirectSound bug workaround:
    //     There is a known issue with volume levels of duplicated buffers.
    //     The duplicated buffer will play at full volume unless you change
    //     the volume to a different value than the original buffer's volume
    //     setting. If the volume stays the same (even if you explicitly set
    //     the same volume in the duplicated buffer with a 
    //     IDirectSoundBuffer8::SetVolume call), the buffer will play at full
    //     volume regardless.
    LONG volume;
    sound.buffer->GetVolume(&volume);
    buffer->SetVolume(volume + 1);
    
    // Creating notification event
    notification = CreateEvent(NULL, 0, 0, NULL);
    ASSERT_DIRECTX(buffer->QueryInterface(IID_IDirectSoundNotify8, (void **)&notification_obj));
    
    // Setting notification
    DSBPOSITIONNOTIFY pn;
    pn.dwOffset = data_size - 1;
    pn.hEventNotify = notification;
    ASSERT_DIRECTX(notification_obj->SetNotificationPositions(1, &pn));
}

// Fill buffer with contents
void D3D_SMBASE::StaticSound::fill()
{
    void * ptr;
    DWORD ptr_data_size;
    
    // Locking entire buffer
    ASSERT_DIRECTX(buffer->Lock(0, 0, &ptr, &ptr_data_size, 
                                NULL, NULL, DSBLOCK_ENTIREBUFFER));
    const DWORD size_to_fill = data_size;
    ASSERT(size_to_fill == ptr_data_size);
    
    // Filling with data
    CopyMemory(ptr, data_ptr, size_to_fill);

    // Unlocking buffer
    buffer->Unlock(ptr, ptr_data_size, NULL, 0);
}

// Process end-of-buffer notification
void D3D_SMBASE::StaticSound::process_end_of_buffer()
{
    // Non-streaming sound reached the end
    if (!(flags & SoundBase::looped) || force_no_loop)
    {
        DWORD pos;
        ASSERT_DIRECTX(buffer->GetCurrentPosition(&pos, NULL));
        const DWORD delta = 20000;
        if (pos >= data_size-delta || (0 <= pos && pos <= delta))
        {
            if (!(flags & SoundBase::looped) || force_no_loop)
            {
                rewind();
                force_no_loop = false;
            }
            is_over = true;
        }
    }
    else
        is_over = true;
    
    // Resetting event
    SoundBase::process_end_of_buffer();
}


// Get play cursor position
float D3D_SMBASE::StaticSound::get_play_pos()
{
    DWORD pos;
    buffer->GetCurrentPosition(&pos, NULL);
    return float(pos) / sound_desc.nAvgBytesPerSec;
}

#undef D3D_SM
#undef D3D_SMBASE
