#pragma once
#include "Tanita2.h"
#include "Graphics.h"
#include "TextureManager.h"
#include "VertexManager.h"
#include "SequenceManager.h"
#include "SoundManager.h"
#include <d3dx9.h>

// Forward references
class Direct3DInstance::SequenceManagerInstance;
namespace ingame
{
    class Gizmo;
    class TextObject;
    class Region;
    class Path;
    class PathFindRegion;
    class GameObject;
    class EndRenderTargetSequence;
}

//! Base class for animation sequence rendering manager
/** This class only defines the SequenceBase for
  * declaring SequenceManagerInstance as manager of
  * SequenceBase instances in RenderManager template */
class Direct3DInstance::SequenceManagerInstanceBase
{
public:
    // Forward reference
    class SequenceID;
    
    //! Base class for animation sequence object
    class SequenceBase
    {
    public:
        //! Constructor
        SequenceBase();
        //! Destructor
        virtual ~SequenceBase() = 0 {}
        
        //! Sequence object oriented bounding box width and height
        D3DXVECTOR2 bounding_box;
    
    protected:
        //! Position (used to calculate transformation matrix)
        D3DXVECTOR2 position;
        
        //! Sequence rendering
        virtual void render() = 0;
        
        //! Called when sprite is to be added to render queue
        /** This method should check if sequence should be rendered
          * and return either true to be added, or false to skip. */
        virtual bool on_adding_to_queue( float dt )
            { update_transformation_matrix(); return true; }
        
        //! Update transformation matrix
        void update_transformation_matrix();
        
        //! Transformation matrix for rendering
        D3DXMATRIX transformation;
        
        //! Frame rate
        int fps;
        //! Current frame number
        int current_frame_number;
        //! Frame count
        int frame_count;
        //! Animation flags
        int flags;
        //! Animation is being played flag
        bool is_playing;
        //! Animation has passed last(first for reversed) frame.
        bool is_over;
        
        enum AnimationFlags
        {
            looped          = 1,   // Animation is looped
            reversed        = 2,   // Animation will be played from end to start
            vertical_flip   = 4,   // Animation will be fliped vertically
            horizontal_flip = 8,   // Animation will be fliped horizontally
        };
        
        // Add new sound
        inline virtual void add_sound( int frame, SoundID & sound ) {}
        // Delete sound
        inline virtual void del_sound( int frame, SoundID & sound ) {}

        // Stop sequence
        virtual void stop() { is_playing = false; }
        // Play sequence
        virtual void play() { is_playing = true; }
        // Set current frame
        virtual void set_frame( int frame ) { current_frame_number = frame; }
        
        // Obtaining texture
        inline virtual TextureRef get_texture() { return TextureRef(); }
        
        // Friend classes
        friend class Direct3DInstance::SequenceManagerInstance;
        friend class SequenceID;
    };
    
    //! Sequence ID proxy object
    class SequenceID
    {
    public:
        //! Constructor
        inline SequenceID() : id(0), s(NULL) {}
        //! Constructor
        /** \param  id  identifier of sequence */
        SequenceID( ID id );
        //! Type conversion
        inline operator ID() { return id; }
        
        //! Update sequence and add to render queue
        /** \param  dt  time step for sequence updating */
        void render( float dt );
        
        //! Get position
        inline D3DXVECTOR2 get_position() const { return s->position; }
        //! Set position
        /** \param  pos  position to set */
        inline void set_position( const D3DXVECTOR2 & pos ) { s->position = pos; }
        
        //! Get fps value
        inline int get_fps() const { return s->fps; }
        //! Set fps value
        inline void set_fps( int fps ) { s->fps = fps; }
        
        //! Get current animation frame number (starting from 0)
        inline int get_current_frame() { return s->current_frame_number; }
        //! Set current frame number
        /** \note When 'reversed' flag is specified, frame number is
          * counted from end to start, thus frame 0 will be the last in
          * non-reversed sequence */
        inline void set_current_frame( int frame )
            { if (frame < 0) frame = 0;
              if (frame >= s->frame_count) frame = s->frame_count-1;
              if (s->flags & s->reversed) frame = s->frame_count - (frame + 1);
              s->is_over = false;
              s->is_playing = true;
              s->set_frame(frame); }
        
        //! Get number of frames
        inline int get_frame_count() { return s->frame_count; }
        
        // Animation flags setting/getting
        #define define_flag(name)                                         \
            inline bool get_##name() { return s->flags & s->##name; }     \
            inline void set_##name( bool value )                          \
                { s->flags &= ~s->##name;                                 \
                  s->flags |= (value ? s->##name : 0); }
        
        define_flag(looped)
        define_flag(reversed)
        define_flag(vertical_flip)
        define_flag(horizontal_flip)
        
        #undef define_flag
        
        //! Check if animation is being played
        inline bool is_playing() { return s->is_playing; }
        //! Resume animation playback
        inline void play() { s->play(); }
        //! Pause animation playback
        inline void stop() { s->stop(); }
        //! Check if animation has passed over last frame
        inline bool is_over() { return s->is_over; }
        
        //! Get sequence object oriented bounding box parameters
        inline D3DXVECTOR2 get_bounding_box() const { return s->bounding_box; }
        
        //! Bind class to python
        static void create_bindings();
        
        //! Check if point lays inside sequence box
        bool is_inside( const D3DXVECTOR2 & p );
        
        //! Texture obtaining
        inline TextureRef get_texture() { return s->get_texture(); }
        
        //! Update transformation matrix
        inline void update_transformation_matrix() { s->update_transformation_matrix(); }
        
        // Add new sound
        inline void add_sound( int frame, SoundID & sound )
            { s->add_sound(frame, sound); }
        // Delete sound
        inline void del_sound( int frame, SoundID & sound )
            { s->del_sound(frame, sound); }
    
    protected:
        //! Sequence identifier
        ID id;
        //! Sequence instance
        SequenceBase * s;
        
        // Friend class
        friend class Direct3DInstance::SequenceManagerInstance;
        friend class ingame::EndRenderTargetSequence;
        friend class ingame::Gizmo;
        friend class ingame::TextObject;
        friend class ingame::Region;
        friend class ingame::Path;
        friend class ingame::PathFindRegion;
    };
};

//! Sequence base class
typedef Direct3DInstance::SequenceManagerInstanceBase::SequenceBase SequenceBase;
//! Sequence identifier definition
typedef Direct3DInstance::SequenceManagerInstanceBase::SequenceID SequenceID;

//! Sequence rendering manager
class Direct3DInstance::SequenceManagerInstance: 
    public Direct3DInstance::SequenceManagerInstanceBase,
    public RenderManager<SequenceBase, SequenceID>
{
public:
    //! Animation sequence manager initialization
    inline SequenceManagerInstance() {}
    //! Cleanup
    inline ~SequenceManagerInstance() {};

	//! Rendering sequences
	void draw_queue();
    
    //! Flushing rendering queue
    void flush();
    
    //! Clear rendering queue without flushing
    inline void clear_queue()
        { RenderManager<SequenceBase, SequenceID>::flush(); }
};
