#include "stdafx.h"
#include "GameObject.h"

using namespace ingame;

// Destructor
AnimatedObject::~AnimatedObject()
{
    SequenceManager sm;
    for (tanita2_dict::iterator i = sequences.begin(); sequences.end() != i; ++i)
        sm->del((SequenceID &)(bp::extract<SequenceID &>(i.value())));
}

// Set state
void AnimatedObject::set_state( bp::str & next_state )
{
// Macro to call callable objects with assertion
#define ingameCALL(func, call_statement)                       \
    if (bp::object() != func)                                      \
    {                                                          \
        python::Python py;                                     \
        ingameASSERT(py->is_callable(func) &&                  \
                     #func ## " function is not callable");    \
        call_statement;                                        \
    }
    
    if (bp::str() != next_state && state != next_state)
    {
        // Calling on_exit of previous state
        if (bp::str() != state)
        {
            State & old_state = bp::extract<State &>(states[state]);
            ingameCALL(old_state.on_exit, old_state.on_exit());
        }
        
        // Changing state
        state = next_state;
                
        // Calling on_enter of current state
        State & new_state = bp::extract<State &>(states[state]);
        // Starting to play current sequence
        if (bp::object() != new_state.sequence)
            sequences[bp::str(new_state.sequence)].attr("frame") = 0;
        ingameCALL(new_state.on_enter, new_state.on_enter());
    }
}

// Loading sequence
void AnimatedObject::add_sequence( bp::str & name, bp::object & sequence_path, bp::tuple & frame_numbers,
                                   bool compressed )
{
    SequenceManager sm;
    std::vector<int> frames;
    for (int i = 0; i < bp::extract<int>(frame_numbers.attr("__len__")()); ++i)
        frames.push_back(bp::extract<int>(frame_numbers[i]));
    ingameTRY(sequences.setitem(name,
        bp::object(sm->add(AnimatedSequence(PATH(bp::extract<char *>(sequence_path)),
		               frames, compressed)))));
}

// Load sequence with sound
void AnimatedObject::add_sound_sequence( bp::str & name, bp::object & sequence_path,
                                         bp::tuple & frame_numbers, bp::tuple & sounds, bool compressed )
{
    SequenceManager sm;
    std::vector<int> frames;
    for (int i = 0; i < bp::extract<int>(frame_numbers.attr("__len__")()); ++i)
        frames.push_back(bp::extract<int>(frame_numbers[i]));
    
    AnimatedSoundSequence::FrameSounds index_sound;
    for (int i = 0; i < bp::extract<int>(sounds.attr("__len__")()); ++i)
        index_sound.push_back(std::make_pair(int(bp::extract<int>(sounds[i][0])), 
            (SoundID)bp::extract<SoundID>(this->sounds[bp::str(sounds[i][1])])));
    ingameTRY(sequences.setitem(name,
        bp::object(sm->add(AnimatedSoundSequence(PATH((char *)bp::extract<char *>(sequence_path)),
		               frames, index_sound, compressed)))));
}

// Loading sequence
void AnimatedObject::add_large_sequence( bp::str & name, bp::object & sequence_path, bp::tuple & frame_numbers,
                                         bool compressed )
{
    SequenceManager sm;
    std::vector<int> frames;
    for (int i = 0; i < bp::extract<int>(frame_numbers.attr("__len__")()); ++i)
        frames.push_back(bp::extract<int>(frame_numbers[i]));
    ingameTRY(sequences.setitem(name,
        bp::object(sm->add(LargeAnimatedSequence(PATH((char *)bp::extract<char *>(sequence_path)),
                                             frames, compressed)))));
}

// Load large sequence with sound
void AnimatedObject::add_large_sound_sequence( bp::str & name, bp::object & sequence_path,
                                               bp::tuple & frame_numbers, bp::tuple & sounds, bool compressed )
{
    SequenceManager sm;
    std::vector<int> frames;
    for (int i = 0; i < bp::extract<int>(frame_numbers.attr("__len__")()); ++i)
        frames.push_back(bp::extract<int>(frame_numbers[i]));
    
    LargeAnimatedSoundSequence::FrameSounds index_sound;
    for (int i = 0; i < bp::extract<int>(sounds.attr("__len__")()); ++i)
        index_sound.push_back(std::make_pair(int(bp::extract<int>(sounds[i][0])), 
            (SoundID)bp::extract<SoundID>(this->sounds[bp::str(sounds[i][1])])));
    ingameTRY(sequences.setitem(name,
        bp::object(sm->add(LargeAnimatedSoundSequence(PATH((char *)bp::extract<char *>(sequence_path)),
		                                          frames, index_sound, compressed)))));
}

// Updating bp::object state
void AnimatedObject::update( float dt )
{
    // If we are inside z-region, we should just update sequence's transformation matrix
    if (is_inside_zregion)
    {
        begin_update();
        State & current_state = bp::extract<State &>(states[state]);
        SequenceID & s = bp::extract<SequenceID &>(sequences[bp::str(current_state.sequence)]);
        s.update_transformation_matrix();
        
        update_children(dt);
        end_update();
        is_inside_zregion = false;
        return;
    }
    
    begin_update();
    update_sounds(dt);
    ingameASSERT(bp::str() != state && "Current state name is None");
    State & current_state = bp::extract<State &>(states[state]);
    
    // Processing links
    if (bp::object() != current_state.link)
    {
        python::Python py;
        ingameASSERT(py->is_callable(current_state.link) && "Link function is not callable");
        bp::object ns = current_state.link();
        if (bp::object() != ns)
            set_state(bp::str(ns));
    }
    
    // Calling on_update function
    char * s = bp::extract<char *>(state);
    ingameCALL(current_state.on_update, current_state.on_update(dt));
#undef ingameCALL
    
    // Playing current sequence
    if (bp::object() != current_state.sequence)
    {
        sequences[bp::str(current_state.sequence)].attr("render")(dt);
    }
    
    // Updating children
    GameObject::update_children(dt);
    end_update();
}
