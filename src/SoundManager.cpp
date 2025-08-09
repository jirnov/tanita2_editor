#include "stdafx.h"
#include "SoundManager.h"
#include "Sound.h"
#include "Application.h"

// Useful definitions
#define D3D_SM DirectSoundInstance::SoundManagerInstance
#define D3D_SMBASE DirectSoundInstance::SoundManagerInstanceBase

// Sound updating
void D3D_SM::update( DWORD dt )
{
    // Updating timestamp
    timestamp += dt;
    
    // Updating timestamps for sounds in queue
    for (int i = 0; queue_end != i; ++i)
    {
        queue[i].s->is_old = false;
        queue[i].s->timestamp = timestamp;
    }
    
    // Clearing queue
    RenderManager<D3D_SMBASE::SoundBase, D3D_SMBASE::SoundID>::flush();
    
    // Updating all currently playing sounds
    for (ObjectIter i = objects.begin(); objects.end() != i; ++i)
    {
        SoundBase & s = *i->second;
        
        if (ApplicationInstance::active)
            s.is_over = false;
        
        // Panning and volume fading
        D3DXVECTOR4 screen_point;
        D3DXVec2Transform(&screen_point, &D3DXVECTOR2(0, 0), &s.transformation);
        float dx = screen_point.x - 512,
              dy = screen_point.y - 384;
        s.set_pan(s.pan + int(dx) * 100 / 2048);

        if (s.is_music)
            s.set_volume(int(music_volume * s.volume));
        else
            s.set_volume(int(sound_volume * s.volume * int(4096 - sqrtf(dx * dx + dy * dy)) / 4096));
        
        // Checking if sound should be stopped
        if (s.is_playing && s.timestamp + dt + 200 < timestamp)
        {
            // Marking sound as old to prevent playback
            s.is_old = true;
            
            // If sound is prolonged, it shouldn't be looped
            if (s.flags & SoundBase::prolonged)
                s.force_no_loop = true;
            else
            {
                // Stopping sound which is not used
                s.stop();
                s.rewind();
            }
        }
        // Processing notifications
        s.process_notifications();
        
        // Checking if sound has reached the end
        if (WAIT_OBJECT_0 == WaitForSingleObject(s.notification, 0))
            s.process_end_of_buffer();
    }
}

// Set music volume
void D3D_SM::set_music_volume( float volume )
{
	music_volume = volume;
}

// Set sound volume
void D3D_SM::set_sound_volume( float volume )
{
	sound_volume = volume;
}

// Load music
SoundID D3D_SM::load_music( PATH & path )
{
    StaticSound * to_duplicate = NULL;

    Config cfg;
    if (!cfg->get<bool>("disable_file_cache"))
    {

    // Checking if sound was loaded before
    for (ObjectIter i = objects.begin(); objects.end() != i; ++i)
        if (i->second->is_static)
        {
            StaticSound & s = dynamic_cast<StaticSound &>(*i->second);
            if (s.path == path)
            {
                // We should duplicate this sound
                to_duplicate = &s;
                break;
            }
        }
        
    }

    if (to_duplicate)
    {
        StaticSound new_sound(*to_duplicate);
        new_sound.timestamp = timestamp;
        new_sound.is_music = true;
        return add(new_sound);
    }
    ApplicationInstance::pause();
    
    StaticSound new_sound(path);
    new_sound.timestamp = timestamp;
    new_sound.is_music = true;
    SoundID id = add(new_sound);

    ApplicationInstance::resume();
    return id;
}


// Load sound
SoundID D3D_SM::load( PATH & path )
{
    StaticSound * to_duplicate = NULL;

    Config cfg;
    if (!cfg->get<bool>("disable_file_cache"))
    {

    // Checking if sound was loaded before
    for (ObjectIter i = objects.begin(); objects.end() != i; ++i)
        if (i->second->is_static)
        {
            StaticSound & s = dynamic_cast<StaticSound &>(*i->second);
            if (s.path == path)
            {
                // We should duplicate this sound
                to_duplicate = &s;
                break;
            }
        }
        
    }

    if (to_duplicate)
    {
        StaticSound new_sound(*to_duplicate);
        new_sound.timestamp = timestamp;
        return add(new_sound);
    }
    ApplicationInstance::pause();
    
    StaticSound new_sound(path);
    new_sound.timestamp = timestamp;
    SoundID id = add(new_sound);

    ApplicationInstance::resume();
    return id;
}

// Bind class to python
void D3D_SMBASE::SoundID::create_bindings()
{
    using namespace bp;

    // Binding SoundID as if it were SoundBase
    class_<SoundID>("Sound")
        .def("render",                 &SoundID::render)
        .add_property("position",      &SoundID::get_position,  &SoundID::set_position)
        
        .add_property("looped",        &SoundID::get_looped,    &SoundID::set_looped)
        .add_property("prolonged",     &SoundID::get_prolonged, &SoundID::set_prolonged)
        
        .add_property("is_playing",    &SoundID::is_playing)
        .add_property("is_over",       &SoundID::is_over)
        
        .def("play",                   &SoundID::play)
        .def("stop",                   &SoundID::stop)
        .def("rewind",                 &SoundID::rewind)
        
        .add_property("volume",        &SoundID::get_volume, &SoundID::set_volume)
        .add_property("pan",           &SoundID::get_pan,    &SoundID::set_pan)
        ;
}

// Constructor
D3D_SMBASE::SoundID::SoundID( ID id ) : id(id)
{
    SoundManager sm;
    s = sm->get(id);
}

// Add sound to render queue
void D3D_SMBASE::SoundID::render( float dt )
{
    SoundManager sm;
    if (s->is_playing && s->on_adding_to_queue(dt))
        sm->add_to_render_queue(*this);
}

// Destructor
D3D_SM::~SoundManagerInstance()
{
    for (ObjectIter i = objects.begin(); objects.end() != i;)
    {
        i->second->unload();
        i = objects.erase(i);
    }
}

#undef D3D_SM
#undef D3D_SMBASE
