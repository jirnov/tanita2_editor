#pragma once
#include "SequenceManager.h"

//! Namespace for game object type declarations
namespace ingame
{

//! GameObject state class
class State
{
public:
    //! Constructor
    inline State() {}
    //! Constructor
    /** \param  sequence       sequence to be rendered in this state
      * \param  on_enter       function to be called when entering to state
      * \param  on_update      function to be called while being in state
      * \param  on_exit        function to be called when leaving state
      * \param  link           function returning state name or None */
    inline State( bp::object sequence, bp::object on_enter = bp::object(), 
                  bp::object on_update = bp::object(), bp::object on_exit = bp::object(),
                  bp::object link = bp::object() )
        : sequence(sequence), on_enter(on_enter), on_update(on_update),
          on_exit(on_exit), link(link) {}
    
protected:
    //! Sequence to be played in this state
    bp::object sequence;

    //! Function to be called on entering to state
    bp::object on_enter;
    //! Function to be called while being in state
    bp::object on_update;
    //! Function to be called on when exiting from state
    bp::object on_exit;
    
    //! Link function object (returns next state name or None)
    bp::object link;
    
    // Friend
    friend class GameObject;
    friend class AnimatedObject;
};

}
