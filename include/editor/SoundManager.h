#pragma once
#include "Tanita2.h"
#include "Sound.h"
#include "FileManager.h"

// Streaming sound buffer length
#define STREAM_BUFFER_LEN_IN_SECONDS 5

namespace ingame
{
    class SoundSequenceBase;
};

//! Base class for sound rendering manager
class DirectSoundInstance::SoundManagerInstanceBase
{
public:
    // Forward reference
    class SoundID;
    
    //! Base class for sound object
    class SoundBase
    {
    public:
        //! Constructor
        SoundBase();
        
        //! Destructor
        virtual ~SoundBase() = 0 {}
    
    protected:
        //! Sound position in space
        D3DXVECTOR2 position;
        //! Transformation matrix for rendering
        D3DXMATRIX transformation;
        
        //! Sound buffer size
        DWORD buffer_size;

        //! Sound playback flags
        int flags;
        //! Sound is being played flag
        bool is_playing;
        //! Sound stopped or is to start again (if looped).
        bool is_over;
        //! Sound is music
        bool is_music;
        
        //! Flag indicating that buffer is static
        bool is_static;
        
        //! Force sound looping (for stream buffers)
        bool force_stream_loop;
        
        //! Sound flags
        enum SoundFlags
        {
            looped    = 1,   // Sound is looped
            prolonged = 2,   // Sound is playing to end even if not in queue
        };

        // Volume (0 - mute, 100 - max)
        int volume;
        // Pan (-100 - left only, 100 - right only)
        int pan;
        
        // Play sound
        void play();
        // Stop (pause) sound
        void stop();
        // Rewind sound to start
        virtual void rewind();
        
        // Set sound
        void set_volume( int new_volume );
        // Set pan
        void set_pan( int new_pan );
        
        //! Sound description
        WAVEFORMATEX sound_desc;
        //! Pointer to DirectSound buffer
        LPDIRECTSOUNDBUFFER buffer;
        // End of playback notification event
        HANDLE notification;
        // Notification object pointer
        LPDIRECTSOUNDNOTIFY8 notification_obj; 

        //! Unloading
        virtual void unload();
        
        //! Called when sound is to be added to render queue
        virtual bool on_adding_to_queue( float dt );
        
        //! Should sound buffer be looped
        virtual bool is_buffer_looped() { return force_stream_loop || (flags & looped); }
        
        //! Process notifications if any
        virtual void process_notifications() {}
        //! Process end-of-buffer notification
        virtual void process_end_of_buffer() { ResetEvent(notification); }
        
        // Sound priority
        int priority;
        // Last update timestamp (in milliseconds)
        DWORD timestamp;
        // Flag to force prolonged sounds be not looped
        bool force_no_loop;
        // Flag indicating that sounds timestamp is old and it shouldn't be played
        bool is_old;
        
        // Get play cursor position
        virtual float get_play_pos() { return 0; }
        
        // Friend classes
        friend class ingame::SoundSequenceBase;
        friend class DirectSoundInstance::SoundManagerInstance;
        friend class SoundID;
    };
    
    //! Static sound
    class StaticSound : public SoundBase
    {
    public:
        //! Constructor
        StaticSound( PATH & path );
        
        //! Copy constructor
        explicit StaticSound( const StaticSound & sound );
    
    protected:
        //! Sound path
        PATH path;
        //! Sound file
        FileRef file;
        //! Pointer to sound data start
        BYTE * data_ptr;
        //! Size of sound data in bytes
        DWORD data_size;
       
        //! Fill buffer with contents
        void fill();
        
        //! Process end-of-buffer notification
        virtual void process_end_of_buffer();
        
        // Get play cursor position
        virtual float get_play_pos();
        
        // Friend classes
        friend class DirectSoundInstance::SoundManagerInstance;
    };
    
    //! Base class for streamed sounds
    class StreamingSoundBase : public SoundBase
    {
    public:
        //! Constructor
        StreamingSoundBase();

        //! Destructor
        virtual ~StreamingSoundBase() = 0 {}
        
    protected:
        //! Creating sound buffer
        void create_buffer();
    
        //! Size of sound data in bytes
        DWORD data_size;
        //! Rest of sound data (non-written)
        DWORD data_rest;
        
        //! Copy data to given buffer
        virtual void copy_data( BYTE * dest, int count ) = 0;
        
        //! Stream buffer update notification events
        HANDLE stream_notification[2];
        //! Streaming buffer lock offset
        DWORD stream_lock_offset;
    
        //! Fill buffer with contents
        void fill();
        //! Fill streaming buffer with contents part
        void fill_streaming();
        
        // Rewind sound to start
        virtual void rewind();
        
        //! Process notifications if any
        virtual void process_notifications();
        //! Process end-of-buffer notification
        virtual void process_end_of_buffer();
        
        //! Should sound buffer be looped
        virtual bool is_buffer_looped( int count ) { return true; }
        
        // Get play cursor position
        virtual float get_play_pos();
        
        // Play buffer fill counter
        DWORD play_offset;
        
        //! Unloading
        virtual void unload();
    };

    //! Sound ID proxy object
    class SoundID
    {
    public:
        //! Constructor
        inline SoundID() : id(0), s(NULL) {}
        //! Constructor
        /** \param  id  identifier of sound */
        SoundID( ID id );
        //! Type conversion
        inline operator ID() { return id; }
        
        //! Update sound and add to render queue
        /** \param  dt  time step for sound updating */
        void render( float dt );
        
        //! Get position
        inline D3DXVECTOR2 get_position() const { return s->position; }
        //! Set position
        /** \param  pos  position to set */
        inline void set_position( const D3DXVECTOR2 & pos ) { s->position = pos; }
        
        // Sound flags setting/getting
        #define define_flag(name)                                         \
            inline bool get_##name() { return s->flags & s->##name; }     \
            inline void set_##name( bool value )                          \
                { s->flags &= ~s->##name;                                 \
                  s->flags |= (value ? s->##name : 0); }
        
        define_flag(prolonged)
        
        #undef define_flag
        
        // Looped flag should be set in other way
        inline bool get_looped() { return s->flags & s->looped; }
        inline void set_looped( bool value )
            { bool is_p = s->is_playing;
              s->stop();
              s->flags &= ~s->looped;
              s->flags |= (value ? s->looped : 0);
              if (is_p) s->play(); }
        
        //! Get volume
        inline int get_volume() const { return s->volume; }
        //! Set volume
        inline void set_volume( int volume ) { s->volume = volume; }
        
        //! Get pan
        inline int get_pan() const { return s->pan; }
        //! Set pan
        inline void set_pan( int pan ) { s->pan = pan; }
        
        //! Get priority
        inline int get_priority() const { return s->priority; }
        //! Set priority
        inline void set_priority( int priority ) { s->priority = priority; }
        
        //! Check if sound is being played
        inline bool is_playing() { return s->is_playing; }
        //! Resume sound playback
        inline void play() { s->play(); }
        //! Pause sound playback
        inline void stop() { s->stop(); }
        //! Rewind sound to start
        inline void rewind() { s->rewind(); }
        //! Check if sound has passed over last frame
        inline bool is_over() { return s->is_over; }
        
        //! Get sound play cursor position
        inline float get_play_pos() { return s->get_play_pos(); }
        
        //! Bind class to python
        static void create_bindings();
        
    protected:
        //! Sequence identifier
        ID id;
        //! Sound instance
        SoundBase * s;
        
        // Friend class
        friend class DirectSoundInstance::SoundManagerInstance;
        friend class ingame::SoundSequenceBase;
    };
};

//! Sound base class
typedef DirectSoundInstance::SoundManagerInstanceBase::SoundBase SoundBase;
//! Sound identifier definition
typedef DirectSoundInstance::SoundManagerInstanceBase::SoundID SoundID;

//! Sound rendering manager
class DirectSoundInstance::SoundManagerInstance: 
    public DirectSoundInstance::SoundManagerInstanceBase,
    public RenderManager<SoundBase, SoundID>
{
public:
    //! Sound manager initialization
    inline SoundManagerInstance() : timestamp(0), sound_volume(1), music_volume(1) {}
    //! Cleanup
    ~SoundManagerInstance();
    
    //! Updating and flushing rendering queue
    void update( DWORD dt );

    //! Set sound volume
    void set_sound_volume( float volume );
    
    //! Set music volume
    void set_music_volume( float volume );
    
    //! Load sound
    SoundID load( PATH & path );

    //! Load music
    SoundID load_music( PATH & path );

    //! Clear rendering queue without flushing
    inline void clear_queue()
        { RenderManager<SoundBase, SoundID>::flush(); }
        
protected:
    // Current timestamp (milliseconds)
    DWORD timestamp;

    // Sound volume
    float sound_volume;

    // Music volume
    float music_volume;
    
    // Friend class
    friend class ingame::SoundSequenceBase;
};
