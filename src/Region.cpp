#include "stdafx.h"
#include "Region.h"

using namespace ingame;

// Iterate through points
bp::object PointContainer::iterate()
{
    bp::list t;
    for (std::vector<D3DXVECTOR2>::iterator i = points.begin(); i != points.end(); ++i)
        t.append(*i);
    return t;
}

// Save points to file
void PointContainer::save( bp::object & path )
{
	FILE * f = fopen(std::string(PATH(bp::extract<char *>(path)).get_path_string()).c_str(), "wb");
    ASSERT(NULL != f);
    
    // Writing signature and element count
    fwrite(&PointContainer::signature, 4, 1, f);
    DWORD count = points.size();
    fwrite(&count, 4, 1, f);
    
    // Writing points
    for (PointsIter i = points.begin(); points.end() != i; ++i)
        fwrite(&D3DXVECTOR2(floorf(i->x), floorf(i->y)), sizeof(D3DXVECTOR2), 1, f);
    fclose(f);
}

// Rebuild cached vertex list
void PointContainer::OutlineSequence::rebuild_cache()
{
    cached_points.resize(points->size()+1);
    for (UINT i = 0; i < points->size(); ++i)
        cached_points[i] = D3DXVECTOR3((*points)[i].x, (*points)[i].y, 1);
    cached_points[cached_points.size() - 1] = cached_points[0];
}

// Sequence rendering
void PointContainer::OutlineSequence::render()
{
    if (cached_points.size() == 0) return;
    
    Direct3D d3d;
    D3DXMATRIX tr = transformation * d3d->get_projection_matrix();
    d3d->line_drawer->SetWidth(1);
    d3d->line_drawer->DrawTransform(&cached_points[0], cached_points.size(),
                                    &tr, color);
}

#define UPDATE_POINTS {on_points_change(); seq->rebuild_cache();}

// Add point to container
void PointContainer::push( const D3DXVECTOR2 & v )
{
    points.push_back(D3DXVECTOR2(floorf(v.x), floorf(v.y)));
    UPDATE_POINTS;
}

#define CHECK_INDEX ingameASSERT(0 <= index && index < points.size())
// Insert point at given position
void PointContainer::insert( unsigned int index, const D3DXVECTOR2 & v )
{
    CHECK_INDEX;
    points.insert(points.begin() + index, D3DXVECTOR2(floorf(v.x), floorf(v.y)));
    UPDATE_POINTS;
}

// Erase point at given position
void PointContainer::erase( unsigned int index )
{
    CHECK_INDEX;
    points.erase(points.begin() + index);
    UPDATE_POINTS;
}

// Get item at given position
D3DXVECTOR2 PointContainer::getitem( unsigned int index )
{
    CHECK_INDEX;
    return points[index];
}

// Set item at given position
void PointContainer::setitem( unsigned int index, const D3DXVECTOR2 & v )
{
    CHECK_INDEX;
    points[index] = D3DXVECTOR2(floorf(v.x), floorf(v.y));
    UPDATE_POINTS;
}

// Load region from file
void PointContainer::load( bp::object & path )
{
    FileManager fm;
    FileRef f = fm->load(PATH(bp::extract<char *>(path)));
    
    // Checking signature
    BYTE * data = (BYTE *)f->get_contents();
    if (*(DWORD *)data != PointContainer::signature)
        throw Exception("Points file has invalid signature (perhaps, old file format)");
    DWORD count = *((DWORD *)data + 1);
    
    // Reading region points
    D3DXVECTOR2 * ptr = (D3DXVECTOR2 *)(data + 8);
    for (UINT i = 0; i < count; ++i)
        points.push_back(*(ptr + i));
    UPDATE_POINTS;
}

#undef CHECK_INDEX
#undef UPDATE_POINTS

// Check if vertical ray intersects the segment
static bool check_vray_segment_intersection( const D3DXVECTOR2 & ray_point,
                                             const D3DXVECTOR2 & a, 
                                             const D3DXVECTOR2 & b )
{
    float left = a.x, right  = b.x,
          top = a.y,  bottom = b.y;
    if (left > right) left = b.x, right  = a.x;
    if (bottom > top) top  = b.y, bottom = a.y;
    
    // Ray lays outside segment's rectangle
    if (ray_point.x >= right || ray_point.x <= left || ray_point.y >= top)
        return false;
    
    // Ray lays under segment's rectangle
    if (ray_point.y < bottom) return true;
    
    // Intersection point
    float dx = b.x - a.x, dy = b.y - a.y;
#define EPSILON 0.0001f // Comparison error

    // Segment is a vertical line - let's say there's no intersection
    if (-EPSILON < dx && dx < EPSILON) return false;
    
    float y = dy / dx * (ray_point.x - a.x) + a.y;
    if (y > ray_point.y) return true;
    
#undef EPSILON
    return false;
}

// Constructor
Region::Region()
{
    SequenceManager sm;
    ingameTRY(sequence = sm->add(OutlineSequence(&points, D3DCOLOR_XRGB(255, 0, 0))));
    seq = dynamic_cast<OutlineSequence *>(sequence.s);
}

// Check if object is inside region
bool Region::is_inside( const GameObject & obj )
{
    D3DXVECTOR4 v_tmp;
    D3DXVec2Transform(&v_tmp, &obj.get_absolute_position(), &transformation_inv);
    return is_local_point_inside(D3DXVECTOR2(v_tmp.x, v_tmp.y));
}

// Updating region
void Region::update( float dt )
{
    begin_update(); // We should remember transformation matrix
	                // for future is_inside checks
    sequence.render(dt);
	update_children(dt); // Update point gizmos
	end_update();
}

// Check if point is inside region (in local coordinates)
bool Region::is_local_point_inside( const D3DXVECTOR2 & v )
{
    // Intersection count (odd if inside)
    int count = 0;
    const D3DXVECTOR2 v_tmp = D3DXVECTOR2(floorf(v.x)+0.5f, floorf(v.y)+0.5f);
    
    // Finding intersection count
    for (UINT i = 0; i < points.size(); ++i)
        // Checking for intersection with vertical line
        if (check_vray_segment_intersection(v_tmp, points[i], points[(i + 1) % points.size()]))
            count++;
    return count & 1;
}

// Check if point (in absolute coordinates) inside region
bool Region::is_point_inside( const D3DXVECTOR2 & p )
{
    D3DXVECTOR4 v_tmp;
    D3DXVec2Transform(&v_tmp, &p, &transformation_inv);
    return is_local_point_inside(D3DXVECTOR2(v_tmp.x, v_tmp.y));
}
