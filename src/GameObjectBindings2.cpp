#include "stdafx.h"
#include "GameObject.h"
#include "Region.h"
#include "Path.h"

using namespace ingame;

// Bind class to python
void GameObject::create_bindings2()
{
    using namespace bp;

    typedef Wrapper<AnimatedObject> AnimatedObjectWrap;
    class_<AnimatedObjectWrap, bases<GameObject> >("AnimatedObject")
        .def("update",              &AnimatedObject::update,  &AnimatedObjectWrap::default_update)

        .def("add_sequence",             &AnimatedObject::add_sequence,
            (arg("name"), arg("path"), arg("frames"), arg("compressed") = true))
        .def("add_sound_sequence",       &AnimatedObject::add_sound_sequence,
            (arg("name"), arg("path"), arg("frames"), arg("sounds"), arg("compressed") = true))
        .def("add_large_sequence",       &AnimatedObject::add_large_sequence,
            (arg("name"), arg("path"), arg("frames"), arg("compressed") = true))
        .def("add_large_sound_sequence", &AnimatedObject::add_large_sound_sequence,
            (arg("name"), arg("path"), arg("frames"), arg("compressed") = true))			
            
        .def_readwrite("sequences",         &AnimatedObject::sequences)
        .def_readwrite("states",            &AnimatedObject::states)
        .def_readwrite("is_inside_zregion", &AnimatedObject::is_inside_zregion)
        
        .add_property("state", &AnimatedObject::get_state, &AnimatedObject::set_state)
        ;
        
    typedef AbstractWrapper<PointContainer> PointContainerWrap;
    class_<PointContainerWrap, boost::noncopyable, bases<GameObject> >("PointContainer")
        .def("__len__",     &Region::len)
        .def("__getitem__", &Region::getitem)
        .def("__setitem__", &Region::setitem)
        .def("insert",      &Region::insert)
        .def("__delitem__", &Region::erase)
        .def("clear",       &Region::clear)
        .def("push",        &Region::push)
        .def("load",        &Region::load)
        .def("__nonzero__", &Region::__nonzero__)
        .def("__iter__",    &Region::iterate)
        
        .def("save",           &Region::save)
        .add_property("color", &Region::get_color, &Region::set_color)
        ;
    
    typedef Wrapper<Region> RegionWrap;
    class_<RegionWrap, bases<PointContainer> >("Region")
        .def("update",      &Region::update,  &RegionWrap::default_update)
        
        .def("is_inside",   &Region::is_inside)
        .def("is_point_inside",  &Region::is_point_inside)
        ;

    Path::create_bindings();
        
    typedef Wrapper<PathFindRegion> PathFindRegionWrap;
    class_<PathFindRegionWrap, bases<Region> >("PathFindRegion")
        .def("update",           &PathFindRegion::update,  &PathFindRegionWrap::default_update)
        .def("find_path",        &PathFindRegion::find_path)
        .def("is_point_inside",  &PathFindRegion::is_point_inside)
        
        .def_readwrite("block_regions", &PathFindRegion::block_regions)
        ;

    typedef Wrapper<Gizmo> GizmoWrap;
    class_<GizmoWrap, bases<GameObject> >("Gizmo")
        .def("update",          &Gizmo::update,     &GizmoWrap::default_update)
        .def("is_inside",       &Gizmo::is_inside)
        
        .add_property("width",  &Gizmo::get_width,  &Gizmo::set_width)
        .add_property("height", &Gizmo::get_height, &Gizmo::set_height)
        .add_property("color",  &Gizmo::get_color,  &Gizmo::set_color)
        ;
    
    typedef Wrapper<TextObject> TextObjectWrap;
    class_<TextObjectWrap, bases<GameObject> >("TextObject")
        .def("update",         &TextObject::update,    &TextObjectWrap::default_update)
        .add_property("text",  &TextObject::get_text,  &TextObject::set_text)
        .add_property("color", &TextObject::get_color, &TextObject::set_color)
        ;
}
