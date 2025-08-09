#pragma once
#include "SequenceManager.h"
#include "GameObject.h"
#include <vector>

//! Namespace for game object type declarations
namespace ingame
{

//! Abstract base class for path and region
class PointContainer: public GameObject
{
public:
    //! Constructor
    inline PointContainer()
        : seq(NULL)
        {}
    
    //! Destructor
    virtual ~PointContainer() = 0 {}
    
protected:
    //! Add point to container
    void push( const D3DXVECTOR2 & v );
    //! Insert point at given position
    void insert( unsigned int index, const D3DXVECTOR2 & v );
    //! Erase point at given position
    void erase( unsigned int index );
    //! Get item at given position
    D3DXVECTOR2 getitem( unsigned int index );
    //! Set item at given position
    void setitem( unsigned int index, const D3DXVECTOR2 & v );
    //! Get number of points
    inline int len() { return points.size(); }
    //! Clear points
    inline void clear() { points.clear(); }

    //! Comparison operator
    inline bool __nonzero__() { return true; }

    //! Load points from file
    void load( bp::object & path );

    //! Function called when points changes
    virtual void on_points_change() {}
    
    //! Iterate through points
    bp::object iterate();
    
    //! Save points to file
    void save( bp::object & path );

    //! Container outlining sequence
    class OutlineSequence: public SequenceBase
    {
    public:
        // Constructor
        inline OutlineSequence() : points(NULL), color(0) {}
        // Constructor
        inline OutlineSequence( std::vector<D3DXVECTOR2> * points, D3DCOLOR color ) 
            : points(points), color(color) {}

    protected:
        // Rendering
        virtual void render();

        // Color
        D3DCOLOR color;
        // Pointer to points vector
        std::vector<D3DXVECTOR2> * points;
        // Cached points
        std::vector<D3DXVECTOR3> cached_points;
        
        // Rebuild points cache
        void rebuild_cache();
        
        friend class PointContainer;
    };
    
    //! Set and get border color
    inline void set_color( DWORD color ) { seq->color = color; }
    inline DWORD get_color() { return seq->color; }
    
    // Outline sequence
    SequenceID sequence;
    OutlineSequence * seq;

    //! Points
    std::vector<D3DXVECTOR2> points;
    typedef std::vector<D3DXVECTOR2>::iterator PointsIter;
    
    // Points file signature
    static const DWORD signature = MAKELONG(MAKEWORD('R', 'G'), MAKEWORD('N', '0'));
    
    // Friend
    friend class GameObject;
};

//! Region class
class Region: public PointContainer
{
public:
    //! Constructor
    Region();

protected:
    //! Check if object is inside region
    bool is_inside( const GameObject & obj );

    //! Update region
    virtual void update( float dt );

    //! Check if point is inside region
    bool is_point_inside( const D3DXVECTOR2 & p );
    
    // Check if point is inside region (in local coordinates)
    bool is_local_point_inside( const D3DXVECTOR2 & p );
    
    // Friends
    friend class GameObject;
    friend class PathFindRegion;
};

}
