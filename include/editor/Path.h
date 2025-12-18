#pragma once
#include "GameObject.h"
#include "micropather.h"
#include "Region.h"

//! Namespace for game object type declarations
namespace ingame
{

//! Path (trajectory) representation
class Path: public PointContainer
{
public:
    //! Constructor
    Path();

protected:
    //! Update path
    virtual void update( float dt );
   
    //! Attach object to path
    void attach( bp::object & obj );
    //! Detach object from path
    inline void detach() { attach(bp::object()); }
    
    //! Stop movement/rotation
    inline void stop() { is_playing = false; }
    //! Resume movement/rotation
    inline void play() { is_playing = true; }

    //! Attached object
    bp::object attached_obj;
    
    //! False if attached object moved over path, True if relative to it
    bool relative_movement;
    //! False if attached object aligned over path, True if relative to it
    bool relative_rotation;
    //! True if path affects object position
    bool affect_position;
    //! True if path affects object rotation (baseline orientation)
    bool affect_rotation;
    //! Reversed direction of movement
    bool reversed;
    
    //! Get reversed flag
    inline bool get_reversed() { return reversed; }
    //! Set reversed flag
    void set_reversed( bool value );
    
    //! Stop/resume flag
    bool is_playing;
    
    // Update for non-reversed movement
    void update_nonreversed( float dt );
    // Update for reversed movement
    void update_reversed( float dt );
    
    // Dynamic parameters for path point
    struct PathPointData
    {
        float speed;
        float angle;
        float dist;
    };
    // Data for path points
    std::vector<PathPointData> point_data;
    typedef std::vector<PathPointData>::iterator PathPointDataIter;
    
    // Distance passed since last point
    float passed_dist;
    // Index of last passed point
    int passed_index;
    // Time passed since last point
    float time_passed;
    
    // Key point class
    struct KeyPoint
    {
        // Constructors
        inline KeyPoint() : speed(0), index(0), reached(false) {}
        inline KeyPoint( int index, float speed )
            : speed(speed), index(index), reached(false) {}
        
        // Speed in pixels/sec
        float speed;
        
        // Index of corresponding point
        int index;
        
        // Flag indicating that point was reached
        bool reached;
    };
    
    // Key points container class
    struct KeyPointContainer
    {
        // __getitem__ method
        inline KeyPoint & KeyPointContainer::getitem( bp::str & id )
            { return kp[id]; }
        // __setitem__ method
        inline void KeyPointContainer::setitem( bp::str & id, KeyPoint & value )
            { kp[id] = value; }
        // __delitem__ method
        inline void KeyPointContainer::delitem( bp::str & id )
            { kp.erase(id); }
        
        // Get list of keys
        inline bp::object KeyPointContainer::iterkeys()
        {
            bp::list t;
            for (KPIter i = kp.begin(); i != kp.end(); ++i)
                t.append(i->first);
            return t;
        }

        // Get list of values
        inline bp::object KeyPointContainer::itervalues()
        {
            bp::list t;
            for (KPIter i = kp.begin(); i != kp.end(); ++i)
                t.append(i->second);
            return t;
        }

        // Get list of key-value pairs
        inline bp::object KeyPointContainer::iteritems()
        {
            bp::list t;
			for (KPIter i = kp.begin(); i != kp.end(); ++i) {
				bp::tuple item = make_tuple(i->first, i->second);
				t.append(item);
			}
            return t;
        }
        
        // Key check
        bool KeyPointContainer::has_key( bp::str & id )
        {
            for (KPIter i = kp.begin(); i != kp.end(); ++i)
                if (id == i->first) return true;
            return false;
        }
        
        // Length
        inline int len() { return kp.size(); }
            
        // Path keypoints
        std::map<bp::str, KeyPoint> kp;
        typedef std::map<bp::str, KeyPoint>::iterator KPIter;
    };
    // Key points
    KeyPointContainer key_points;
    
    // Resetting path object if points were changed
    virtual void on_points_change()
        { attached_obj = bp::object(); key_points.kp.clear(); }
    
    //! Outlining sequence
    class PathOutlineSequence: public PointContainer::OutlineSequence
    {
    protected:
        // Constructor
        inline PathOutlineSequence() {}
        // Constructor
        inline PathOutlineSequence( std::vector<D3DXVECTOR2> * points, D3DCOLOR color ) 
            : PointContainer::OutlineSequence(points, color) {}
    
        // Rendering
        virtual void render();
        
        // Friend classes
        friend class Path;
        friend class PathFindRegion;
    };

    // Friend
    friend class GameObject;
    friend class PathFindRegion;
    
    // Bind class to python
    static void create_bindings();
};

//! Region with path finding capability
class PathFindRegion: public Region
{
public:
    //! Constructor
    PathFindRegion();
    //! Destructor
    virtual ~PathFindRegion() { if (pather) delete pather; }
    
protected:
    //! Get path from point A to point B (or empty path if none)
    Path find_path( const D3DXVECTOR2 & A, const D3DXVECTOR2 & B );
    
    //! Function called when points changes
    virtual void on_points_change();
    
    //! List of block regions
    bp::list block_regions;
    
private:
    // MicroPather Graph interface implementation
    class RegionMap: public micropather::Graph
    {
    public:
        // Constructor
        inline RegionMap() : width(0), height(0) {}
    
        // Return least cost estimation between two states
        virtual float LeastCostEstimate( void * start, void * end );
        // Fill adjacency array for given state
        virtual void AdjacentCost( void * state,
                                   std::vector<micropather::StateCost> * adjacent );
        // Print state in readable form
        virtual void PrintStateInfo( void * state ) {}
        
        // Nodes list type definition
        typedef std::vector<int> NodeList;
        typedef std::vector<int>::iterator NodeIter;
        
        // List of node coordinates
        NodeList nodes;
        // Map width and height
        int width, height;
        // Nodes offset
        int left, down;
        // Grid step size
        static const int grid_size = 32;
    };
    // Map object used in path finding
    RegionMap map;
    // MicroPather object
    micropather::MicroPather * pather;
    
    // Helper function for path straighting
    bool is_straight( int ax, int ay, int bx, int by );
    
    //! Outlining sequence
    class PathFindOutline: public PointContainer::OutlineSequence
    {
    protected:
        // Constructor
        inline PathFindOutline() : map(NULL) {}
        // Constructor
        inline PathFindOutline( std::vector<D3DXVECTOR2> * points, D3DCOLOR color,
                                RegionMap * map )
            : PointContainer::OutlineSequence(points, color), map(map) {}
    
        // Rendering
        virtual void render();
        
        // Region map pointer
        RegionMap * map;
        
        // Friend classes
        friend class Path;
        friend class PathFindRegion;
        
        // Vertex format
        struct PathOutlineFormat
        {
            // Position
            float x, y, z;
            // Color
            DWORD color;

            // Constructor
            PathOutlineFormat( float x, float y, DWORD color )
                : x(x), y(y), z(0), color(color) {}

                // Flexible vertex format description
                static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
        };
        // Cached region points
        std::vector<PathOutlineFormat> cached_node_points;
    };
    
    // Friend
    friend class GameObject;
};

}
