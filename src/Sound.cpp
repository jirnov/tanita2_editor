#include "stdafx.h"
#include "SoundManager.h"
#include "Sound.h"
#include "Config.h"
#include "Application.h"

// Initializes DirectSound API and rendering infrastructure
DirectSoundInstance::DirectSoundInstance()
{
    Log log;
    Config conf;
    
    // Creating DirectSound object
    ASSERT_DIRECTX(DirectSoundCreate8(&DSDEVID_DefaultPlayback, &dsound, NULL));
    ASSERT_DIRECTX(dsound->SetCooperativeLevel(ApplicationInstance::window, DSSCL_PRIORITY));
    
    // Getting device capabilities
    ZeroMemory(&device_caps, sizeof(DSCAPS));
    device_caps.dwSize = sizeof(DSCAPS); 
    ASSERT_DIRECTX(dsound->GetCaps(&device_caps));
    {
        std::string primary_16 = (device_caps.dwFlags & DSCAPS_PRIMARY16BIT) ? "yes" : "no",
                    secondary_16 = (device_caps.dwFlags & DSCAPS_SECONDARY16BIT) ? "yes" : "no",
                    primary_stereo = (device_caps.dwFlags & DSCAPS_PRIMARYSTEREO) ? "yes" : "no",
                    secondary_stereo = (device_caps.dwFlags & DSCAPS_SECONDARYSTEREO) ? "yes" : "no";
        log->print(boost::str(boost::format(
                       "Sound device capabilities: "
                       "16 bit sound (primary: %1%, secondary: %2%), "
                       "stereo (primary: %3%, secondary: %4%). "
                       "Hardware mixing buffers: %5%")
                       % primary_16 % secondary_16 % primary_stereo % secondary_stereo
                       % device_caps.dwMaxHwMixingAllBuffers));
    }
    
    
    // Obtaining primary buffer
    LPDIRECTSOUNDBUFFER primary_buffer = NULL;
    DSBUFFERDESC descr;
    ZeroMemory(&descr, sizeof(DSBUFFERDESC));
    descr.dwSize = sizeof(DSBUFFERDESC);
    descr.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
    descr.lpwfxFormat = NULL;
    ASSERT_DIRECTX(dsound->CreateSoundBuffer(&descr, &primary_buffer, NULL));

    // Setting primary buffer format
    WAVEFORMATEX wfx;
    ZeroMemory(&wfx, sizeof(WAVEFORMATEX));
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = 2;
    wfx.nSamplesPerSec  = conf->get<int>("sound_sample_rate");
    wfx.wBitsPerSample  = conf->get<int>("sound_bits_per_sample");
    wfx.nBlockAlign     = wfx.wBitsPerSample / 8 * wfx.nChannels;
    wfx.nAvgBytesPerSec = (DWORD)wfx.nSamplesPerSec * wfx.nBlockAlign;
    ASSERT_DIRECTX(primary_buffer->SetFormat(&wfx));
    primary_buffer->Release();
    
    // Creating sound manager
    sound_manager.create();
}

// DirectSound cleanup
DirectSoundInstance::~DirectSoundInstance()
{
#define SAFE_RELEASE(res) { if (res) res->Release(); }
    // Releasing DirectSound resources
    sound_manager.~sound_manager();

    // Releasing DirectSound instances
    SAFE_RELEASE(dsound);
#undef SAFE_RELEASE
}

// Updating sound system
void DirectSoundInstance::update( DWORD dt )
{
    sound_manager->update(dt);
}

// Clear sound queue
void DirectSoundInstance::clear_render_queue()
{
    sound_manager->clear_queue();
}
