#include "stdafx.h"
#include "GameObject.h"
#include "Region.h"
#include "Path.h"
#include "Helpers.h"

using namespace ingame;

// Adding/changing child object
void GameObject::objects_dict::setitem( bp::str & index, bp::object & value )
{
    python::Python py;
    value.attr("parent") = py["weakref"].attr("proxy")(bp::make_function(&objects_dict::get_parent,
        bp::return_internal_reference<>())(this));
    
    value.attr("__dict__")["name"] = index;
    bp::object constructor = py->getattr(value, "construct");
    if (bp::object() != constructor)
        constructor();
    tanita2_dict::setitem(index, value);
}

// Bind class to python
void GameObject::create_bindings()
{
    using namespace bp;

	typedef Wrapper<GameObject> GameObjectWrap;
	class_<GameObjectWrap>("GameObject")
        .def("update",             &GameObject::update,  &GameObjectWrap::default_update)
        .def("begin_update",       &GameObject::begin_update)
        .def("update_children",    &GameObject::update_children)
        .def("update_sounds",      &GameObject::update_sounds)
        .def("end_update",         &GameObject::end_update)
        
        .def("add_sound",           &GameObject::add_sound,           (arg("name"), arg("path")))
        
        .def("to_local_coordinates", &GameObject::to_local_coordinates)

        .def_readwrite("objects",  &GameObject::objects)
        .def_readwrite("sounds",   &GameObject::sounds)
        .def_readwrite("position", &GameObject::position)
        .def_readwrite("scale",    &GameObject::scale)
        .def_readwrite("rotation", &GameObject::rotation)
        
        .add_property("absolute_position", &GameObject::get_absolute_position)
        .add_property("absolute_rotation", &GameObject::get_absolute_rotation)
		;
        
    class_<objects_dict, bases<tanita2_dict> >("objects_dict")
        .def("__setitem__",   &objects_dict::setitem, with_custodian_and_ward<1,2>())
        ;
        
    typedef Wrapper<Location> LocationWrap;
    class_<LocationWrap, bases<GameObject> >("Location")
        .def("update",             &Location::update, &LocationWrap::default_update)
        .def_readwrite("name",     &Location::name)
        .def_readwrite("width",    &Location::width)
        .def_readwrite("height",   &Location::height)
        ;
        
    typedef Wrapper<Layer> LayerWrap;
    class_<LayerWrap, bases<GameObject> >("Layer")
        .def("update",                 &Layer::update, &LayerWrap::default_update)
        .def_readwrite("parallax",     &Layer::parallax)
        ;
    
    typedef Wrapper<LayerImage> LayerImageWrap;
    class_<LayerImageWrap, bases<GameObject> >("LayerImage")
        .def("update",              &LayerImage::update,  &LayerImageWrap::default_update)
        .def("load_image",          &LayerImage::load_image, (arg("path"), arg("compressed") = true))
        .def("direct_load_image",   &LayerImage::direct_load_image, (arg("path"), arg("compressed") = true))
        .def_readwrite("sequence",  &LayerImage::sequence)
        ;
        
    class_<State>("State", init<object, object, object, object, object>(
                               (arg("sequence"), arg("on_enter") = object(), 
                                arg("on_update") = object(), arg("on_exit") = object(),
                                arg("link") = object())))
        .def_readwrite("sequence",  &State::sequence)
        .def_readwrite("on_enter",  &State::on_enter)
        .def_readwrite("on_update", &State::on_update)
        .def_readwrite("on_exit",   &State::on_exit)
        .def_readwrite("link",     &State::link)
        ;
    create_bindings2();
}
