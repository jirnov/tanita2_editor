#include "stdafx.h"
#include "Sequence.h"

using namespace ingame;

// Constructor
SoundSequenceBase::SoundSequenceBase( FrameSounds & asounds )
    : current_sound(NULL), old_position(0)
{
    for (FrameSoundsIter i = asounds.begin(); asounds.end() != i; ++i)
    {
        bool found = false;
        for (FrameSoundsExIter j = attached_sounds.begin(); attached_sounds.end() != j; ++j)
            if (j->first == i->first)
            {
                j->second.push_back(i->second);
                found = true;
                break;
            }
        if (found) continue;
        attached_sounds.push_back(std::pair<int, FrameSoundArray>(i->first, FrameSoundArray()));
        attached_sounds.back().second.push_back(i->second);
    }
}

// Add new sound
void SoundSequenceBase::add_sound( int frame, SoundID & sound )
{
    for (FrameSoundsExIter j = attached_sounds.begin(); attached_sounds.end() != j; ++j)
        if (j->first == frame)
        {
            j->second.push_back(sound);
            return;
        }
    attached_sounds.push_back(std::pair<int, FrameSoundArray>(frame, FrameSoundArray()));
    attached_sounds.back().second.push_back(sound);
}

// Delete sound
void SoundSequenceBase::del_sound( int frame, SoundID & sound )
{
    for (FrameSoundsExIter j = attached_sounds.begin(); attached_sounds.end() != j; ++j)
        if (j->first == frame)
            for (FrameSoundArrayIter i = j->second.begin(); j->second.end() != i; ++i)
                if (*i == sound)
                {
                    j->second.erase(i);
                    if (j->second.size() == 0)
                        attached_sounds.erase(j);
                    return;
                }
    ASSERT(false && "Sound not found");
}

// Stop sequence
void SoundSequenceBase::stop()
{
    for (FrameSoundsExIter j = attached_sounds.begin(); attached_sounds.end() != j; ++j)
        for (FrameSoundArrayIter i = j->second.begin(); j->second.end() != i; ++i)
            if (!i->get_prolonged())
                i->rewind();
}

// Updating
void SoundSequenceBase::update_sound_queue( float dt, int previous_frame,
                                            int current_frame )
{
    if (previous_frame == current_frame) return;
    
    SoundManager sm;
    if (previous_frame < current_frame)
    {
        for (FrameSoundsExIter i = attached_sounds.begin(); attached_sounds.end() != i; ++i)
            if (previous_frame <= i->first && i->first < current_frame)
            {
                SoundID & s = i->second[rand() % i->second.size()];
                s.s->is_old = false;
                s.s->timestamp = sm->timestamp;
                s.rewind();
                s.play();
                current_sound = s.s;
                old_position = 0;
            }
    }
    else
        for (FrameSoundsExIter i = attached_sounds.begin(); attached_sounds.end() != i; ++i)
            if (previous_frame <= i->first || i->first < current_frame)
            {
                SoundID & s = i->second[rand() % i->second.size()];
                s.s->is_old = false;
                s.s->timestamp = sm->timestamp;
                s.rewind();
                s.play();
                current_sound = s.s;
                old_position = 0;
            }
}

// Calculate dt from current sound
float SoundSequenceBase::calculate_dt( float dt )
{
    return dt;
}

// Updating
bool AnimatedSoundSequence::on_adding_to_queue( float dt )
{
    int old_frame = current_frame_number;
    dt = calculate_dt(dt);
    bool result = AnimatedSequence::on_adding_to_queue(dt);
    
    update_sound_queue(dt, old_frame, current_frame_number);
    return result;
}

// Updating
bool LargeAnimatedSoundSequence::on_adding_to_queue( float dt )
{
    int old_frame = current_frame_number;
    dt = calculate_dt(dt);
    bool result = LargeAnimatedSequence::on_adding_to_queue(dt);

    update_sound_queue(dt, old_frame, current_frame_number);
    return result;
}
