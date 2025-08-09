#pragma once
#include "Tanita2.h"
#include "SequenceManager.h"
#include <vector>

// Namespace for game object type declarations
namespace ingame
{

//! One-frame (static) sequence
class StaticSequence: public SequenceBase
{
public:
    //! Constructor
    inline StaticSequence() {}
    //! Creating static sequence for given texture
    StaticSequence( PATH & texture_path, bool compressed );

protected:
    //! Rendering
    virtual void render();
    
    // Obtaining texture
    virtual TextureRef get_texture() { return texture; }

    // Reference to texture
    TextureRef texture;
    // Reference to vertex buffer
    VertexBufferRef vbuffer;
};

//! Base class for animated sequence
class AnimatedSequenceBase: public SequenceBase
{
public:
    //! Constructor
    AnimatedSequenceBase( std::vector<int> & frame_indices );
    //! Destructor
    virtual ~AnimatedSequenceBase() = 0 {}
        
protected:
    //! Updating
    virtual bool on_adding_to_queue( float dt );
	
	// Time left for current frame
	float time_left;
};

//! Animated sequence
class AnimatedSequence: public AnimatedSequenceBase
{
public:
    //! Creating sequence for given path and collection of frame image indices
	AnimatedSequence( PATH & path, std::vector<int> & frame_indices, bool compressed );
    
protected:
    //! Rendering
    virtual void render();
    
    // Frame description
    struct FrameInfo
    {
        TextureRef      texture;
        VertexBufferRef buffer;
    };
    // Frames
    std::vector<FrameInfo> frames;
    
    // Obtaining texture
    virtual TextureRef get_texture() { return frames[current_frame_number].texture; }
};

//! Base class for sequences with sound
class SoundSequenceBase
{
public:
    //! Type for sounds attached to frame indices
    typedef std::vector<std::pair<int, SoundID> > FrameSounds;
    typedef FrameSounds::iterator FrameSoundsIter;
    
    //! Constructor
    SoundSequenceBase( FrameSounds & attached_sounds );
    
    //! Destructor
    virtual inline ~SoundSequenceBase() = 0 {}
    
    //! Updating
    void update_sound_queue( float dt, int previous_frame,
                             int current_frame );

    // Add new sound
    void add_sound( int frame, SoundID & sound );
    // Delete sound
    void del_sound( int frame, SoundID & sound );

    // Stop sequence
    void stop();
    
protected:
    typedef std::vector<std::pair<int, std::vector<SoundID> > > FrameSoundsEx;
    typedef FrameSoundsEx::iterator FrameSoundsExIter;
    typedef std::vector<SoundID> FrameSoundArray;
    typedef std::vector<SoundID>::iterator FrameSoundArrayIter;
    
    // Current sound for dt calculation
    SoundBase * current_sound;
    
    // Calculate dt from current sound
    float calculate_dt( float dt );
    DWORD old_position;
    
    //! Sounds
    FrameSoundsEx attached_sounds;
};

//! Animated sequence with sound
class AnimatedSoundSequence: public AnimatedSequence, public SoundSequenceBase
{
public:
    //! Creating sequence for given path, collection of frame image indices and 
	inline AnimatedSoundSequence( PATH & path, std::vector<int> & frame_indices,
	                              FrameSounds & attached_sounds, bool compressed )
	    : AnimatedSequence(path, frame_indices, compressed),
          SoundSequenceBase(attached_sounds) {}
    
protected:
    //! Updating
    virtual bool on_adding_to_queue( float dt );
    
    // Add new sound
    inline virtual void add_sound( int frame, SoundID & sound )
        { SoundSequenceBase::add_sound(frame, sound); }
    // Delete sound
    inline virtual void del_sound( int frame, SoundID & sound )
        { SoundSequenceBase::del_sound(frame, sound); }

    // Stop sequence
    virtual void stop() { is_playing = false; SoundSequenceBase::stop(); }
    // Set current frame
    virtual void set_frame( int frame )
        { current_frame_number = frame; SoundSequenceBase::stop(); }
};

//! Animated sequence for large animations
class LargeAnimatedSequence: public AnimatedSequenceBase
{
public:
    //! Creating sequence for given path and collection of frame indices
    LargeAnimatedSequence( PATH & path, std::vector<int> & frame_indices, bool compressed );

protected:
    //! Rendering
    virtual void render();
    //! Updating
    virtual bool on_adding_to_queue( float dt );
    
    // Obtaining texture
    virtual TextureRef get_texture() { return texture; }

    // Frames
    std::vector<FileRef> frames;
    // Dynamic texture
    TextureRef      texture;
    // vertex buffer
    VertexBufferRef buffer;
    // Texture width and height
    int width, height;
    
    // Index of frame loaded to texture
    int loaded_frame;
    
    // Pointer to Direct3D texture
    Direct3DInstance::TextureManagerInstance::DynamicTextureInstance * lowlevel_texture;
    
    // Fill texture with current frame image
    void fill_texture();
};

//! Large animated sequence with sound
class LargeAnimatedSoundSequence: public LargeAnimatedSequence, public SoundSequenceBase
{
public:
    //! Creating sequence for given path, collection of frame image indices and 
    inline LargeAnimatedSoundSequence( PATH & path, std::vector<int> & frame_indices,
                                       FrameSounds & attached_sounds, bool compressed )
        : LargeAnimatedSequence(path, frame_indices, compressed),
          SoundSequenceBase(attached_sounds) {}

protected:
    //! Updating
    virtual bool on_adding_to_queue( float dt );
    
    // Add new sound
    inline virtual void add_sound( int frame, SoundID & sound )
        { SoundSequenceBase::add_sound(frame, sound); }
    // Delete sound
    inline virtual void del_sound( int frame, SoundID & sound )
        { SoundSequenceBase::del_sound(frame, sound); }

    // Stop sequence
    virtual void stop() { is_playing = false; SoundSequenceBase::stop(); }
    // Set current frame
    virtual void set_frame( int frame )
        { current_frame_number = frame; SoundSequenceBase::stop(); }
};

//! Gizmo sequence
class GizmoSequence: public SequenceBase
{
public:
    //! Constructor
    inline GizmoSequence() : color(0) {}
    //! Constructor
    inline GizmoSequence( int width, int height, D3DCOLOR color ) 
        : color(color) { bounding_box = D3DXVECTOR2(float(width), float(height)); }
   
    //! Color
    D3DCOLOR color;
    
protected:
    //! Rendering
    virtual void render();
};

//! Text rendering sequence
class TextSequence: public SequenceBase
{
public:
    //! Constructor
    inline TextSequence() : color(0) {}
    //! Constructor
    inline TextSequence( D3DCOLOR color ) : color(color) {}
    
    //! Text string
    std::string text;
    
    //! Color
    D3DCOLOR color;
    
protected:
    //! Rendering
    virtual void render();
};

} // End of ::ingame namespace
