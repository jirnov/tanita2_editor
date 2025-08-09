#pragma once
#include "Tanita2.h"
#include "Templates.h"
#include <mmsystem.h>
#include <dsound.h>

//! Sound rendering manager
class DirectSoundInstance
{
public:
    //! Initializes DirectSound API and rendering infrastructure
    DirectSoundInstance() throw(DirectXException_);
    //! DirectSound cleanup
    ~DirectSoundInstance();

    //! Updating sound system
    void update( DWORD dt );

    // Sound rendering manager
    class SoundManagerInstanceBase;
    class SoundManagerInstance;
    
    // SoundManagerInstance singleton definition
    typedef Singleton<SoundManagerInstance> SoundManager;

    //! Device capabilities
    DSCAPS device_caps;
    
    //! Clear sound queue
    void clear_render_queue();
    
    //! DirectSound object
    LPDIRECTSOUND8 dsound;
    
protected:
    //! Sound manager
    SINGLETON(SoundManager) sound_manager;
};
//! DirectSound singleton definition
typedef Singleton<DirectSoundInstance> DirectSound;

//! Sound manager
typedef DirectSoundInstance::SoundManager SoundManager;
