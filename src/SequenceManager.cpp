#include "stdafx.h"
#include "SequenceManager.h"
#include "Log.h"
#include <d3dx9.h>

#define D3D_SM Direct3DInstance::SequenceManagerInstance
#define D3D_SMBASE Direct3DInstance::SequenceManagerInstanceBase

// Constructor
D3D_SMBASE::SequenceBase::SequenceBase()
    : position(0, 0), fps(1), current_frame_number(-1),
      frame_count(0), flags(0), is_playing(false), is_over(false),
      bounding_box(0, 0)
{}

// Transformation matrix update
void D3D_SMBASE::SequenceBase::update_transformation_matrix()
{
    const bool hflip = (flags & horizontal_flip),
               vflip = (flags & vertical_flip);
    D3DXMATRIX local_transform;
    
    // Flip handling
    if (vflip || hflip)
    {
        D3DXMATRIX flip_matrix;
        
        D3DXMatrixTranslation(&local_transform, position.x + (hflip ? bounding_box.x : 0.0f),
                                                position.y + (vflip ? bounding_box.y : 0.0f), 0.0f);
        D3DXMatrixScaling(&flip_matrix, hflip ? -1.0f : 1.0f, vflip ? -1.0f : 1.0f, 1.0f);
        local_transform = flip_matrix * local_transform;
    }
    else
        D3DXMatrixTranslation(&local_transform, position.x, position.y, 0.0f);
    
    Direct3D d3d;
    transformation = d3d->get_transform();
    transformation = local_transform * transformation;
}

// Constructor
D3D_SMBASE::SequenceID::SequenceID( ID id )
    : id(id)
{
    SequenceManager sm;
    s = sm->get(id);
}

// Add sequence to render queue
void D3D_SMBASE::SequenceID::render( float dt )
{
    SequenceManager sm;
    if (s->on_adding_to_queue(dt))
		sm->add_to_render_queue(*this);
}

// Check if point lays inside sequence bounding box
bool D3D_SMBASE::SequenceID::is_inside( const D3DXVECTOR2 & p )
{
    Direct3D d3d;
    D3DXMATRIX m = s->transformation;
    
    D3DXVECTOR2 one(0, 0), two(float(s->bounding_box.x),
                               float(s->bounding_box.y));
    D3DXVECTOR4 v1, v2;
    D3DXVec2Transform(&v1, &one, &m);
    D3DXVec2Transform(&v2, &two, &m);

#define SWAP(a, b) { float tmp = a; a = b; b = tmp; }
    if (v1.x > v2.x) SWAP(v1.x, v2.x);
    if (v1.y > v2.y) SWAP(v1.y, v2.y);
#undef SWAP
    return v1.x < float(p.x) && v2.x > float(p.x) && 
           v1.y < float(p.y) && v2.y > float(p.y);
}

// Bind class to python
void D3D_SMBASE::SequenceID::create_bindings()
{
    using namespace bp;

    // Binding SequenceID as if it were Sequence
    class_<SequenceID>("Sequence")
        .def("render",                    &SequenceID::render)
        .def("is_inside",                 &SequenceID::is_inside)
        .add_property("position",         &SequenceID::get_position, &SequenceID::set_position)
        .add_property("bounding_box",     &SequenceID::get_bounding_box)
        .add_property("fps",              &SequenceID::get_fps, &SequenceID::set_fps)
        .add_property("frame",            &SequenceID::get_current_frame, &SequenceID::set_current_frame)
        .add_property("looped",           &SequenceID::get_looped, &SequenceID::set_looped)
        .add_property("reversed",         &SequenceID::get_reversed, &SequenceID::set_reversed)
        .add_property("vertical_flip",    &SequenceID::get_vertical_flip, &SequenceID::set_vertical_flip)
        .add_property("horizontal_flip",  &SequenceID::get_horizontal_flip, &SequenceID::set_horizontal_flip)
        
        .add_property("frame_count",      &SequenceID::get_frame_count)
        
        .add_property("is_playing",       &SequenceID::is_playing)
        .add_property("is_over",          &SequenceID::is_over)
        .def("play",                      &SequenceID::play)
        .def("stop",                      &SequenceID::stop)
        
        .def("add_sound",                 &SequenceID::add_sound)
        .def("del_sound",                 &SequenceID::del_sound)
        ;
}

// Sequence rendering
void D3D_SM::draw_queue()
{
    for (int i = 0; queue_end != i; ++i)
        queue[i].s->render();
}


// Flushing rendering queue
void D3D_SM::flush()
{
    // Rendering sequences
    draw_queue();
    // Clearing queue
    RenderManager<D3D_SMBASE::SequenceBase, D3D_SMBASE::SequenceID>::flush();
}

#undef D3D_SM
#undef D3D_SMBASE
