#include "stdafx.h"
#include "Path.h"
#pragma warning(disable: 4312)

using namespace ingame;
using namespace micropather;

// Bind class to python
void Path::create_bindings()
{
    using namespace bp;

    class_<KeyPoint>("KeyPoint", init<int, float>())
        .def(init<>())
        .def_readwrite("index",  &KeyPoint::index)
        .def_readwrite("speed",  &KeyPoint::speed)
        
        .def_readonly("reached", &KeyPoint::reached)
        ;

    class_<KeyPointContainer>("KeyPointList")
        .def("__getitem__", &KeyPointContainer::getitem, return_internal_reference<>())
        .def("__setitem__", &KeyPointContainer::setitem, with_custodian_and_ward<1,2>())
        .def("__delitem__", &KeyPointContainer::delitem)
        .def("__len__",     &KeyPointContainer::len)
        
        .def("has_key",     &KeyPointContainer::has_key)
        .def("iterkeys",    &KeyPointContainer::iterkeys)
        .def("itervalues",  &KeyPointContainer::itervalues)
        .def("iteritems",   &KeyPointContainer::iteritems)
        ;

    typedef Wrapper<Path> PathWrap;
    class_<PathWrap, bases<PointContainer> >("Path")
        .def("update", &Path::update,  &PathWrap::default_update)
        .def("attach", &Path::attach)
        .def("detach", &Path::detach)
        
        .def("stop", &Path::stop)
        .def("play", &Path::play)
        
        .def_readonly("is_playing", &Path::is_playing)
        .def_readonly("key_points", &Path::key_points)
        
        .def_readwrite("affect_position",   &Path::affect_position)
        .def_readwrite("affect_rotation",   &Path::affect_rotation)
        .def_readwrite("relative_movement", &Path::relative_movement)
        .def_readwrite("relative_rotation", &Path::relative_rotation)

        .add_property("reversed", &Path::get_reversed, &Path::set_reversed)
        ;
}

// Constructor
Path::Path() : affect_position(true),   affect_rotation(true), 
               relative_movement(true), relative_rotation(true),
               reversed(false), is_playing(false)
{
    SequenceManager sm;
    ingameTRY(sequence = sm->add(PathOutlineSequence(&points, D3DCOLOR_XRGB(255, 0, 0))));
    seq = dynamic_cast<PathOutlineSequence *>(sequence.s);
}

// Updating path
void Path::update( float dt )
{
    // Rendering sequence
    begin_update();
    sequence.render(dt);
	update_children(dt);
	end_update();

    // Resetting 'reached' flag for all points
    if (is_playing && bp::object() == attached_obj)
    {
        for (KeyPointContainer::KPIter i = key_points.kp.begin(); key_points.kp.end() != i; ++i)
            i->second.reached = false;
        is_playing = false;
    }

    // Check if path really should be updated
    if (!is_playing || bp::object() == attached_obj ||
        (!affect_position && !affect_rotation))
        return;
    
    begin_update();
    reversed ? update_reversed(dt) : update_nonreversed(dt);
    end_update();
}

// Update for non-reversed movement
void Path::update_nonreversed( float dt )
{
    // Resetting previously reached flags
    for (KeyPointContainer::KPIter i = key_points.kp.begin(); key_points.kp.end() != i; ++i)
        if (i->second.index <= passed_index)
            i->second.reached = false;
    
    // Moving object
    if (affect_position)
    {
        // Old values
        int old_passed_index = passed_index;
        float old_passed_dist = passed_dist;
        
        do
        {
            // Calculating acceleration
            const float dv = point_data[passed_index + 1].speed - point_data[passed_index].speed,
                        v0 = point_data[passed_index].speed;
            const float a = (2 * v0 * dv + dv * dv) / (2 * point_data[passed_index+1].dist);
            
            // Runge-Kutta integration coefficients
            const float k1 = (point_data[passed_index].speed + a * time_passed) * dt;
            const float k2 = (point_data[passed_index].speed + k1 / 2 + (a * (time_passed + dt / 2))) * dt;
            const float k3 = (point_data[passed_index].speed + k2 / 2 + (a * (time_passed + dt / 2))) * dt;
            const float k4 = (point_data[passed_index].speed + k3 + (a * (time_passed + dt))) * dt;
            
            // Integration
            passed_dist += (k1 + 2 * k2 + 2 * k3 + k4) / 6;
            time_passed += dt;
            
            // Break if we have not moved beyond next point
            if (passed_dist < point_data[passed_index+1].dist)
                break;
            
            // Moving to next pair of points
            passed_dist -= point_data[passed_index+1].dist;
            time_passed = 0;
            
            // Updating key point reached flags
            ++passed_index;
            for (KeyPointContainer::KPIter i = key_points.kp.begin(); key_points.kp.end() != i; ++i)
                if (i->second.index == passed_index)
                    i->second.reached = true;
            
            // Detaching object if reached the end
            if (passed_index >= (int)points.size()-1)
            {
                attached_obj = bp::object();
                return;
            }
        } while (true);
        
        // Calculating new position
        float t = passed_dist / point_data[passed_index+1].dist;
        D3DXVECTOR2 pos = points[passed_index + 1] * t + points[passed_index] * (1 - t);
        
        // Moving attached object
        GameObject & attached = bp::extract<GameObject &>(attached_obj());
        if (relative_movement)
        {
            float old_t = old_passed_dist / point_data[old_passed_index+1].dist;
            D3DXVECTOR2 old_pos = points[old_passed_index + 1] * old_t +
                                  points[old_passed_index] * (1 - old_t);
            
            attached.position += pos - old_pos;
        }
        else
        {
            D3DXVECTOR2 dp = get_absolute_position() + pos - attached.get_absolute_position();
            attached.position += dp;
        }
    }
}

// Update for reversed movement
void Path::update_reversed( float dt )
{
    // Resetting previously reached flags
    for (KeyPointContainer::KPIter i = key_points.kp.begin(); key_points.kp.end() != i; ++i)
        if (i->second.index >= passed_index)
            i->second.reached = false;
    
    // Moving object
    if (affect_position)
    {
        // Old values
        int old_passed_index = passed_index;
        float old_passed_dist = passed_dist;
        
        do
        {
            // Calculating acceleration
            const float dv = point_data[passed_index-1].speed - point_data[passed_index].speed,
                        v0 = point_data[passed_index].speed;
            const float a = (2 * v0 * dv + dv * dv) / (2 * point_data[passed_index].dist);
            
            // Runge-Kutta integration coefficients
            const float k1 = (point_data[passed_index].speed + a * time_passed) * dt;
            const float k2 = (point_data[passed_index].speed + k1 / 2 + (a * (time_passed + dt / 2))) * dt;
            const float k3 = (point_data[passed_index].speed + k2 / 2 + (a * (time_passed + dt / 2))) * dt;
            const float k4 = (point_data[passed_index].speed + k3 + (a * (time_passed + dt))) * dt;
            
            // Integration
            passed_dist -= (k1 + 2 * k2 + 2 * k3 + k4) / 6;
            time_passed += dt;
            
            // Break if we have not moved beyond next point
            if (passed_dist > 0)
                break;
            
            // Moving to next pair of points
            --passed_index;
            passed_dist += point_data[passed_index].dist;
            time_passed = 0;
            
            // Updating key point reached flags
            for (KeyPointContainer::KPIter i = key_points.kp.begin(); key_points.kp.end() != i; ++i)
                if (i->second.index == passed_index)
                    i->second.reached = true;
            
            // Detaching object if reached the end
            if (passed_index <= 0)
            {
                attached_obj = bp::object();
                return;
            }
        } while (true);
        
        // Calculating new position
        float t = 1 - passed_dist / point_data[passed_index].dist;
        D3DXVECTOR2 pos = points[passed_index - 1] * t + points[passed_index] * (1 - t);
        
        // Moving attached object
        GameObject & attached = bp::extract<GameObject &>(attached_obj());
        if (relative_movement)
        {
            float old_t = 1 - old_passed_dist / point_data[old_passed_index].dist;
            D3DXVECTOR2 old_pos = points[old_passed_index - 1] * old_t +
                                  points[old_passed_index] * (1 - old_t);
            
            attached.position += pos - old_pos;
        }
        else
        {
            D3DXVECTOR2 dp = get_absolute_position() + pos - attached.get_absolute_position();
            attached.position += dp;
        }
    }
}

// Set reversed flag
void Path::set_reversed( bool value )
{
    reversed = value;
    passed_index += value ? 1 : -1;
}

// Attaching object to path
void Path::attach( bp::object & obj )
{
    // Checking arguments
    if (points.empty() || bp::object() == obj)
    {
        attached_obj = bp::object();
        return;
    }
    // Updating points data
    const UINT points_size = points.size();
    point_data.resize(points_size);
    
    // Attaching object
    python::Python py;
    attached_obj = py["weakref"].attr("ref")(obj);
    
    // Calculating path data for every path point
    int start_index = 0;
    float previous_speed = 0.0f;
    KeyPointContainer::KPIter kp;
    for (kp = key_points.kp.begin(); key_points.kp.end() != kp; ++kp)
    {
        // Starting point is reached by definition
        kp->second.reached = ((reversed ? points_size-1 : 0) == kp->second.index);
        
        float dist = 0;
        ASSERT(kp->second.index < (int)points_size);
        
        // Calculating distance between keypoints
        for (int i = start_index; i < kp->second.index + 1; ++i)
            if (0 == i)
                point_data[i].dist = 0;
            else
            {
                float dx = points[i].x - points[i-1].x,
                      dy = points[i].y - points[i-1].y;
                point_data[i].dist = sqrtf(dx * dx + dy * dy);
                dist += point_data[i].dist;
            }

        // Calculating speed and rotation angles for points
        if (0.001 < dist)
        {
            float dist_passed = 0;
            for (int i = start_index; i < kp->second.index; ++i)
            {
                dist_passed += point_data[i].dist;
                
                float t = (start_index == i) ? 0 : dist_passed / dist;
                point_data[i].speed = kp->second.speed * t + previous_speed * (1 - t);
            }
        }
        previous_speed = kp->second.speed;
        start_index = kp->second.index;
    }
    for (UINT i = start_index; i < points_size; ++i)
    {
        kp->second.reached = ((reversed ? points_size-1 : 0) == kp->second.index);
        if (0 == i)
            point_data[i].dist = 0;
        else
        {
            float dx = points[i].x - points[i-1].x,
                  dy = points[i].y - points[i-1].y;
            point_data[i].dist = sqrtf(dx * dx + dy * dy);
        }
        point_data[i].speed = previous_speed;
    }
    
    // Enabling path handling
    is_playing = true;
    
    // Misc initializations
    passed_index = 0;
    passed_dist = 0;
    time_passed = 0;
    if (reversed)
    {
        passed_index = points_size - 1;
        passed_dist = point_data[passed_index].dist;
        
        GameObject & attached = bp::extract<GameObject &>(attached_obj());
        
        if (relative_movement)
            attached.position += points[passed_index] - points[0];
        else
        {
            D3DXVECTOR2 dp = get_absolute_position() + points[passed_index] - 
                             attached.get_absolute_position();
            attached.position += dp;
        }
    }
}

// Sequence rendering
void Path::PathOutlineSequence::render()
{
    if (cached_points.size() == 0) return;

    Direct3D d3d;
    D3DXMATRIX tr = transformation * d3d->get_projection_matrix();
    d3d->line_drawer->SetWidth(1);
    d3d->line_drawer->DrawTransform(&cached_points[0], cached_points.size()-1,
                                    &tr, color);
}

// Sequence rendering
void PathFindRegion::PathFindOutline::render()
{
    PointContainer::OutlineSequence::render();
    
    if (map->nodes.size() == 0) return;

    Direct3D d3d;

    // Saving previous state
    DWORD oldFVF;
    LPDIRECT3DVERTEXBUFFER9 oldStreamData;
    UINT oldOffsetInBytes, oldStride;
    ASSERT_DIRECTX(d3d->device->GetStreamSource(0, &oldStreamData, &oldOffsetInBytes, &oldStride));
    ASSERT_DIRECTX(d3d->device->GetFVF(&oldFVF));
    DWORD psize;
    ASSERT_DIRECTX(d3d->device->GetRenderState(D3DRS_POINTSIZE, &psize));
    
    // Rendering
    ASSERT_DIRECTX(d3d->device->SetRenderState(D3DRS_POINTSIZE, 2));
    ASSERT_DIRECTX(d3d->device->SetFVF(PathOutlineFormat::FVF));
    ASSERT_DIRECTX(d3d->device->SetTexture(0, NULL));
    d3d->device->SetTransform(D3DTS_WORLD, &transformation);
    ASSERT_DIRECTX(d3d->device->DrawPrimitiveUP(D3DPT_POINTLIST, cached_node_points.size(),
                   &cached_node_points[0], sizeof(PathOutlineFormat)));
    
    // Restoring previous state
    ASSERT_DIRECTX(d3d->device->SetRenderState(D3DRS_POINTSIZE, psize));
    ASSERT_DIRECTX(d3d->device->SetStreamSource(0, oldStreamData, oldOffsetInBytes, oldStride));
    ASSERT_DIRECTX(d3d->device->SetFVF(oldFVF));
    if (oldStreamData) oldStreamData->Release();
}

// Return least cost estimation between two states
float PathFindRegion::RegionMap::LeastCostEstimate( void * start, void * end )
{
    int w = width * grid_size;
    int spos = int(start), sx = spos % w / grid_size, sy = spos / w / grid_size,
        epos = int(end),   ex = epos % w / grid_size, ey = epos / w / grid_size;
    return float(max(abs(ex - sx), abs(ey - sy)));
}

// Fill adjacency array for given state
void PathFindRegion::RegionMap::AdjacentCost( void * state, 
                                              std::vector<micropather::StateCost> * adjacent )
{
    int w = width * grid_size;
    int pos = int(state), xx = pos % w / grid_size, yy = pos / w / grid_size;
    
#define NEIGHBOURS 1
    float costs[NEIGHBOURS*2+1][NEIGHBOURS*2+1] = {{1.4f, 2, 1.4f}, {1, 0, 1}, {1.4f, 2, 1.4f}};

    for (int j = -NEIGHBOURS; j < NEIGHBOURS+1; ++j)
        for (int i = -NEIGHBOURS; i < NEIGHBOURS+1; ++i)
        {
            int x = xx+i, y = yy+j;
            
            if ((i == 0 && j == 0) || x < 0 || y < 0 || x >= width || y >= height ||
                3 != nodes[y * width + x])
                continue;

  #if NEIGHBOURS > 1
            if (i < -1 || i > 1 || j < -1 || j > 1)
            {
                int di = i < -1 ? 1 : i > 1 ? -1 : 0,
                    dj = j < -1 ? 1 : j > 1 ? -1 : 0;
                if (3 != nodes[(y+dj) * width + x + di]) continue;
            }
  #endif
            
            StateCost s;
            s.state = (void *)((y * w + x) * grid_size);
            s.cost = costs[NEIGHBOURS+j][NEIGHBOURS+i];
            adjacent->push_back(s);
        }
}

// Constructor
PathFindRegion::PathFindRegion() : pather(NULL)
{
    pather = new micropather::MicroPather(&map, 1024);
    // Creating sequence for node rendering
    SequenceManager sm;
    sm->del(sequence);
    ingameTRY(sequence = sm->add(PathFindOutline(&points, D3DCOLOR_XRGB(255, 0, 0), &map)));
    seq = dynamic_cast<PathFindOutline *>(sequence.s);
}

// Get path from point A to point B (or empty path if none)
Path PathFindRegion::find_path( const D3DXVECTOR2 & A, const D3DXVECTOR2 & B )
{
#define TOINT(a) (int(a) / RegionMap::grid_size * RegionMap::grid_size)
#define TOSTATE(x, y) (void *)(TOINT(y) * map.width * RegionMap::grid_size + TOINT(x))

    // Transforming points to local coordinates
    D3DXVECTOR4 va_tmp, vb_tmp;
    D3DXVec2Transform(&va_tmp, &A, &transformation_inv);
    D3DXVec2Transform(&vb_tmp, &B, &transformation_inv);
    D3DXVECTOR2 va(va_tmp.x - float(map.left), va_tmp.y - float(map.down)),
                vb(vb_tmp.x - float(map.left), vb_tmp.y - float(map.down));

    // Checking if destination point is blocked
    bool need_find_destination = false;
    if (!is_local_point_inside(D3DXVECTOR2(vb_tmp.x, vb_tmp.y)))
        need_find_destination = true;
    int block_reg_count = bp::extract<int>(block_regions.attr("__len__")());
    for (int i = 0; i < block_reg_count; ++i)
    {
        Region & r = bp::extract<Region &>(block_regions[i]);
        if (r.is_point_inside(B))
        {
            need_find_destination = true;
            break;
        }
    }
    
    // Updating node data (marking nodes blocked by regions with 1,
    // non-blocked with 3, nodes outside region by 0)
    for (int y = 0; y < map.height; ++y)
        for (int x = 0; x < map.width; ++x)
        {
            int & node = map.nodes[map.width * y + x];
            if (!node) continue;
            
            D3DXVECTOR2 p(float(map.left + x * RegionMap::grid_size), 
                          float(map.down + y * RegionMap::grid_size));
            p += get_absolute_position();
            
            if (0 == block_reg_count)
                node = 3; // Point is non-blocked by definition
            else
                // Testing block regions
                for (int i = 0; i < block_reg_count; ++i)
                {
                    Region & r = bp::extract<Region &>(block_regions[i]);
                    node = r.is_point_inside(p) ? 1 : 3;
                }
        }
    
    int ax = int(va.x) / RegionMap::grid_size,
        ay = int(va.y) / RegionMap::grid_size,
        bx = int(vb.x) / RegionMap::grid_size,
        by = int(vb.y) / RegionMap::grid_size;
    
    // Finding destination point inside region
    if (need_find_destination)
    {
        bool found = false;
        
        // Vertical line
        if (0 <= bx && bx < map.width)
        {
            int Dy = by - ay, dy = -1;
            if (Dy < 0) Dy = -Dy, dy = -dy;
            
            const int by_clamp = by < 0 ? 0 : by,
                      vlimit = dy > 0 ? map.height - by_clamp : by_clamp;
            for (int j = 0; j < vlimit; ++j)
            {
                const int new_by = by_clamp + j * dy;
                if (new_by < 0 || new_by >= map.height)
                    continue;
                
                if (map.nodes[map.width * new_by + bx] == 3)
                {
                    const float vby = (float)new_by * RegionMap::grid_size,
                                vb_tmpy = vby + float(map.down);
                    D3DXVECTOR2 point(vb_tmp.x, vb_tmpy);
                    if (is_local_point_inside(point))
                    {
                        D3DXVECTOR4 pt4;
                        D3DXVec2Transform(&pt4, &point, &transformation);
                        D3DXVECTOR2 point_transformed(pt4.x, pt4.y);

                        bool blocked = false;
                        for (int i = 0; i < block_reg_count; ++i)
                        {
                            Region & r = bp::extract<Region &>(block_regions[i]);
                            if (r.is_point_inside(point_transformed))
                            {
                                blocked = true;
                                break;
                            }
                        }
                        if (!blocked)
                        {
                            vb.y = vby;
                            vb_tmp.y = vb_tmpy;
                            by = int(vby) / RegionMap::grid_size;
                            found = true;
                            break;
                        }
                    }
                }
            }
        }

        // Horizontal line
        if (!found && 0 <= by && by < map.height)
        {
            int Dx = bx - ax, dx = -1;
            if (Dx < 0) Dx = -Dx, dx = -dx;
            
            const int bx_clamp = bx < 0 ? 0 : bx,
                      hlimit = dx > 0 ? map.width - bx_clamp : bx_clamp;
            for (int j = 0; j < hlimit; ++j)
            {
                const int new_bx = bx_clamp + j * dx;
                if (new_bx < 0 || new_bx >= map.width)
                    continue;
                
                if (map.nodes[map.width * by + new_bx] == 3)
                {
                    const float vbx = (float)new_bx * RegionMap::grid_size,
                                vb_tmpx = vbx + float(map.left);
                    D3DXVECTOR2 point(vb_tmpx, vb_tmp.y);
                    if (is_local_point_inside(point))
                    {
                        D3DXVECTOR4 pt4;
                        D3DXVec2Transform(&pt4, &point, &transformation);
                        D3DXVECTOR2 point_transformed(pt4.x, pt4.y);

                        bool blocked = false;
                        for (int i = 0; i < block_reg_count; ++i)
                        {
                            Region & r = bp::extract<Region &>(block_regions[i]);
                            if (r.is_point_inside(point_transformed))
                            {
                                blocked = true;
                                break;
                            }
                        }
                        if (!blocked)
                        {
                            vb.x = vbx;
                            vb_tmp.x = vb_tmpx;
                            bx = int(vbx) / RegionMap::grid_size;
                            found = true;
                            break;
                        }
                    }
                }
            }
        }
        // Horizontal line (at A level)
        if (!found && 0 <= ay && ay < map.height)
        {
            int Dx = bx - ax, dx = -1;
            if (Dx < 0) Dx = -Dx, dx = -dx;
            
            const int bx_clamp = bx < 0 ? 0 : bx,
                      hlimit = dx > 0 ? map.width - bx_clamp : bx_clamp;
            for (int j = 0; j < hlimit; ++j)
            {
                const int new_bx = bx_clamp + j * dx;
                if (new_bx < 0 || new_bx >= map.width)
                    continue;
                
                if (map.nodes[map.width * ay + new_bx] == 3)
                {
                    const float vbx = (float)new_bx * RegionMap::grid_size,
                                vb_tmpx = vbx + float(map.left);
                    if (is_local_point_inside(D3DXVECTOR2(vb_tmpx, va_tmp.y)))
                    {
                        vb.x = vbx;
                        vb.y = va.y;
                        vb_tmp.x = vb_tmpx;
                        vb_tmp.y = va_tmp.y;
                        bx = int(vbx) / RegionMap::grid_size;
                        by = ay;
                        found = true;
                        break;
                    }
                }
            }
        }
        if (!found)
            return Path();
    }
    
    // Finding nearest non-blocked grid points for A and B
    int nearest_ax = ax, 
        nearest_bx = bx,
        
        nearest_ay = ay,
        nearest_by = by,
        
        best_dist_a = 100,
        best_dist_b = 100;
    
    int dy = (va.y > vb.y) ?  1 : -1,
        y0 = (dy > 0)      ? -1 :  1,
        y1 = (dy > 0)      ?  2 : -2,
        
        dx = (va.x > vb.x) ?  1 : -1,
        x0 = (dx > 0)      ? -1 :  1,
        x1 = (dx > 0)      ?  2 : -2;

    for (int j = y0; j != y1; j += dy)
        for (int i = x0; i != x1; i += dx)
        {
            int xa = ax + i, ya = ay + j;
            if (xa >= 0 && ya >= 0 && xa < map.width && ya < map.height &&
                map.nodes[map.width * ya + xa] == 3 && abs(i) + abs(j) < best_dist_a)
            {
                best_dist_a = abs(i) + abs(j);
                nearest_ax = xa;
                nearest_ay = ya;
            }
            
            int xb = bx - i, yb = by - j;
            if (xb >= 0 && yb >= 0 && xb < map.width && yb < map.height &&
                map.nodes[map.width * yb + xb] == 3 && abs(i) + abs(j) < best_dist_b)
            {
                best_dist_b = abs(i) + abs(j);
                nearest_bx = xb;
                nearest_by = yb;
            }
        }
    ax = nearest_ax * RegionMap::grid_size;
    ay = nearest_ay * RegionMap::grid_size;
    
    bx = nearest_bx * RegionMap::grid_size;
    by = nearest_by * RegionMap::grid_size;
 
    // Trying to solve path finding problem
    pather->Reset();
    std::vector<void *> path;
    float cost;
    int result = pather->Solve(TOSTATE(ax, ay), TOSTATE(bx, by), &path, &cost);
    if (MicroPather::NO_SOLUTION == result) return Path();
    
#undef TOSTATE
#undef TOINT
    
    // Returning empty path if ends are near
    if (MicroPather::START_END_SAME == result)
    {
        Path p;
        p.push(D3DXVECTOR2(va_tmp.x, va_tmp.y) + position);
        p.push(D3DXVECTOR2(vb_tmp.x, vb_tmp.y) + position);
        return p;
    }
    
    // Returning optimized path
    Path p;
    p.push(D3DXVECTOR2(va_tmp.x, va_tmp.y) + position);
    
    // Straightening path
    int sax = int(va_tmp.x), say = int(va_tmp.y);
    UINT k = 0;
    
    if (is_straight(sax, say, int(vb_tmp.x), int(vb_tmp.y)))
    {
        p.push(D3DXVECTOR2(vb_tmp.x, vb_tmp.y) + position);
        return p;
    }
    
    while (k < path.size()-1)
    {
        int w = map.width * RegionMap::grid_size,
            pos = int(path[k]),
            posn = int(path[k+1]);
        ++k;
        if (is_straight(sax, say, posn % w + map.left, posn / w + map.down))
            continue;
        
        p.push(D3DXVECTOR2(float(pos % w) + float(map.left), float(pos / w) + float(map.down)) + position);
        sax = pos % w + map.left;
        say = pos / w + map.down;
    }
    p.push(D3DXVECTOR2(vb_tmp.x, vb_tmp.y) + position);
    return p;
}

// Helper function for path straighting
bool PathFindRegion::is_straight( int ax, int ay, int bx, int by )
{
    int N = abs(ax - bx);
    if (abs(ay - by) > N)
        N = abs(ay - by);
    N /= RegionMap::grid_size / 2;
    if (0 == N) return true;
    int dx = (bx - ax) / N,
        dy = (by - ay) / N;
    const D3DXVECTOR2 abs_pos = get_absolute_position();

    int block_reg_count = bp::extract<int>(block_regions.attr("__len__")());
    for (int i = 1; i < N-1; ++i)
    {
        float x = float(ax + dx * i),
              y = float(ay + dy * i);
        D3DXVECTOR2 v = D3DXVECTOR2(x, y) + abs_pos;
    
        if (!is_local_point_inside(D3DXVECTOR2(x, y)))
            return false;
        for (int j = 0; j < block_reg_count; ++j)
        {
            Region & r = bp::extract<Region &>(block_regions[j]);
            if (r.is_point_inside(v))
                return false;
        }
    }
    return true;
}

// Function called when points changes
void PathFindRegion::on_points_change()
{
    const int inf = 0x7fffffff;
    map.left = map.down = inf;
    int right = -inf, up = -inf;
    for (PointsIter i = points.begin(); points.end() != i; ++i)
    {
        int x = int(i->x), y = int(i->y);
        
        if (x < map.left) map.left = x;
        if (x > right) right = x;
        if (y < map.down) map.down = y;
        if (y > up) up = y;
    }
    
    map.width = (right - map.left) / RegionMap::grid_size + 1;
    map.height = (up - map.down) / RegionMap::grid_size + 1;
    map.nodes.resize(map.width * map.height);
    for (int y = 0; y < map.height; ++y)
        for (int x = 0; x < map.width; ++x)
            map.nodes[map.width * y + x] = is_local_point_inside(D3DXVECTOR2(
                                               float(map.left + x * RegionMap::grid_size), 
                                               float(map.down + y * RegionMap::grid_size))) ? 1 : 0;
    
    PathFindOutline * s = dynamic_cast<PathFindOutline *>(seq);
    s->cached_node_points.clear();
    s->cached_node_points.reserve(map.nodes.size());
    for (int y = 0; y < map.height; ++y)
        for (int x = 0; x < map.width; ++x)
            if (map.nodes[map.width * y + x])
                s->cached_node_points.push_back(PathFindOutline::PathOutlineFormat(
                    float(map.left + x * RegionMap::grid_size), 
                    float(map.down + y * RegionMap::grid_size), 0xff003000));
}
