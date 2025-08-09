#ifndef _EDITOR_GAMEOBJECT_H_
#define _EDITOR_GAMEOBJECT_H_

#include "Graphics.h"
#include "Sequence.h"
#include "States.h"
#include "Helpers.h"

//! Namespace for game object type declarations
namespace ingame
{

// Exception handling macro
#define ingameTRY(expr)                                                    \
    try { expr; }                                                          \
    catch (std::exception & e)                                             \
    {                                                                      \
        Log log;                                                           \
        log->error(e.what());                                              \
        PyErr_SetString(PyExc_RuntimeError, e.what());                     \
        bp::throw_error_already_set();                                 \
    }                                                                      \
    catch (...)                                                            \
    {                                                                      \
        PyErr_SetString(PyExc_RuntimeError,                                \
            boost::str(boost::format(                                      \
                "Unhandled exception, file '%s', line %d") %               \
                    Exception_::strip_path(__FILE__) % __LINE__).c_str()); \
        bp::throw_error_already_set();                                 \
    }

// Assertion macro for python code
#define ingameASSERT(expr)

// Forward reference
class Path;

//! Base class for all ingame objects
class GameObject
{
public:
    //! Constructor
    GameObject();
    
    //! Bind class to python
    static void create_bindings();
    // Bind class to python (second part)
    static void create_bindings2();
    
    //! Updating states of child objects
    /** This method calls begin_update,
      * then updates all children and sounds and
      * calls end_update.
      * \param  dt  time step */
    virtual void update( float dt );
    
    // Load effect from file
    void load_effect( char * path, char * default_technique );
    // Set effect technique
    void set_effect_technique( char * technique );

    //! Load sound
    void add_sound( bp::str & name, bp::object & sound_path );

    //! Destructor
    virtual ~GameObject() {}
    
    //! Get position in absolute coordinates
    D3DXVECTOR2 get_absolute_position() const;
    //! Get absolute rotation (in degrees)
    float get_absolute_rotation() const;

protected:
    //! Class for automatic management of children classes
    class objects_dict: public tanita2_dict
    {
    public:
        // Constructor
        inline objects_dict() : parent(NULL) {}
        
        //! Adding/changing child object
        void setitem( bp::str & index, bp::object & value );
        
        // Parent object
        GameObject * parent;
        inline GameObject * get_parent() { return parent; }
    };

    //! Begin updating (apply transformations)
    /** This method should be called before any 
      * updates to apply object transformations 
      * correctly. After updating end_update
      * should be called */
    void begin_update();
    //! End update (restore transformations)
    /** This method should be called at end of 
      * update to restore current transformations
      * modified for updating of this object */
    void end_update();
    //! Update child objects
    /** Should be called between begin_update and end_update
      * to ensure proper coordinates */
    void update_children( float dt );
    //! Update sounds
    /** Should be called between begin_update and end_update
      * to ensure proper coordinates */
    void update_sounds( float dt );
    
    //! Position
    D3DXVECTOR2 position;
    //! Rotation angle (in degrees)
    float rotation;
    //! Scaling factor
    D3DXVECTOR2 scale;
    
    //! Child objects
    objects_dict objects;
    
    //! Convert world coordinates to local
    /** Should be called inside begin_update/end_update brackets */
    D3DXVECTOR2 to_local_coordinates( const D3DXVECTOR2 & v );
    
    //! Cached transformation matrix
    D3DXMATRIX transformation;
    //! Inverse transformation matrix
    D3DXMATRIX transformation_inv;
    
    //! Sounds
    tanita2_dict sounds;
    
    // Handle to common parameters
    D3DXHANDLE param_worldview;
    D3DXHANDLE param_proj;
    D3DXHANDLE param_worldviewproj;
    D3DXHANDLE param_time;
    D3DXHANDLE param_dt;
    D3DXHANDLE param_texture;
    D3DXHANDLE param_texture_size;

    // Friend class
    friend class Path;
};

//! Location class
class Location: public GameObject
{
public:
    //! Constructor
    inline Location() : width(1024), height(768) {}

protected:
    //! Location name
    bp::str name;
    
    //! Location width and height
    int width, height;
    
    // Friend class
    friend class GameObject;
};

//! Layer class
class Layer: public GameObject
{
public:
    //! Constructor
    inline Layer() : parallax(0.0f, 0.0f) {}

protected:
    //! Parallax scroll speed
    D3DXVECTOR2 parallax;
    
    // Friend class
    friend class GameObject;
};

//! Layer background image object
class LayerImage: public GameObject
{
public:
    //! Cleanup
    virtual ~LayerImage();

    //! Rendering image
    virtual void update( float dt );

protected:
    //! Load image
    void load_image( bp::object & texture_path, bool compressed );

    //! Load image from direct path
    void direct_load_image( bp::object & texture_path, bool compressed );

    //! Layer image
    SequenceID sequence;
    
    // Friend class
    friend class GameObject;
};

//! Object with states and animations
class AnimatedObject: public GameObject
{
protected:
    //! Constructor
    inline AnimatedObject() : is_inside_zregion(false)
        { states.parent = this; }
    
    //! Destructor
    virtual ~AnimatedObject();

    //! Load sequence
    void add_sequence( bp::str & name, bp::object & sequence_path, bp::tuple & frame_numbers,
                       bool compressed );
    //! Load sequence with sound
    void add_sound_sequence( bp::str & name, bp::object & sequence_path, bp::tuple & frame_numbers,
                             bp::tuple & sounds, bool compressed );
    
	//! Load large sequence
	void add_large_sequence( bp::str & name, bp::object & sequence_path, bp::tuple & frame_numbers,
	                         bool compressed );
	//! Load large sequence with sounds
	void add_large_sound_sequence( bp::str & name, bp::object & sequence_path, bp::tuple & frame_numbers,
	                               bp::tuple & sounds, bool compressed );

    //! Updating object
    virtual void update( float dt );

    //! Sequences
    tanita2_dict sequences;
    //! States
    objects_dict states;
    
    //! Current state
    bp::str state;
    
    //! Set state
    void set_state( bp::str & new_state );
    //! Get current state
    inline bp::str get_state() { return state; }
    
    //! Flag indicating that object is inside z region
    bool is_inside_zregion;
    
    // Friend class
    friend class GameObject;
};

//! Debug render gizmo
class Gizmo: public GameObject
{
public:
    //! Constructor
    Gizmo();
    //! Destructor
    virtual ~Gizmo() { SequenceManager sm; sm->del(sequence); }
    
    //! Rendering
    virtual void update( float dt );

protected:
    //! Set and get gizmo width
    inline void set_width( float width ) { gs->bounding_box.x = width; }
    inline float get_width() { return gs->bounding_box.x; }

    //! Set and get gizmo height
    inline void set_height( float height ) { gs->bounding_box.y = height; }
    inline float get_height() { return gs->bounding_box.y; }
    
    //! Set and get text color
    inline void set_color( DWORD color ) { gs->color = color; }
    inline DWORD get_color() { return gs->color; }
    
    //! Check if point is inside gizmo
    inline bool is_inside( const D3DXVECTOR2 & p )
        { return sequence.is_inside(p); }

    //! Special sequence
    SequenceID sequence;
    //! Gizmo sequence instance pointer
    GizmoSequence * gs;

    // Friend class
    friend class GameObject;
};

//! On-screen debug text object
class TextObject: public GameObject
{
public:
    //! Constructor
    TextObject();
    //! Destructor
    virtual ~TextObject() { SequenceManager sm; sm->del(sequence); }

    //! Rendering
    virtual void update( float dt );

protected:
    //! Set and get text
    inline void set_text( const std::string & text ) { ts->text = text; }
    inline std::string get_text() { return ts->text; }
    
    //! Set and get color
    inline void set_color( DWORD color ) { ts->color = color; }
    inline DWORD get_color() { return ts->color; }

    //! Special sequence
    SequenceID sequence;
    //! Text sequence instance pointer
    TextSequence * ts;

    // Friend class
    friend class GameObject;
};

} // End of ::ingame namespace

#endif // _EDITOR_GAMEOBJECT_H_
