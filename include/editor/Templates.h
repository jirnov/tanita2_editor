#ifndef _EDITOR_TEMPLATES_H_
#define _EDITOR_TEMPLATES_H_

#include "Types.h"

//! %Singleton pattern with manual creation option.
/** %Singleton template with availability of creating
  * parameter instance manually. Used for declaring
  * singleton class in other classes without implicit
  * initialization before explicit creation in constructor.
  *
  * Example:
  * \code
  *   class MyInstance
  *   {
  *   public:
  *       void a();
  *       int b;
  *   };
  *   
  *   void test_myinstance()
  *   {
  *       ThinSingleton<MyInstance> my; // At this point no instance of
  *                                     // MyInstance class exists
  *       my.create(); // Explicit instantiation
  *      
  *       // Member access:
  *       my->a();
  *       my->b = 0;
  *
  *       // When function ends MyInstance will be automatically freed
  *       // if there are no more references to it.
  *   }
  * \endcode
  */
template<class T> class ThinSingleton
{
public:
    //! Manually create singleton parameter instance
    inline void create()
    {
        ref++;
        if (NULL != instance) return;
        ASSERT(!is_second_life);
        instance = new T;
    }

    //! Destructor
    /** Decrements the reference counter and destroys
      * instance when there are no more references. */
    inline ~ThinSingleton()
    {
        if (0 != --ref) return;
        delete instance;
        instance = NULL;
        is_second_life = true;
    }

    //! Access to instance properties and methods.
    inline T * operator ->()
        { _ASSERT(NULL != instance); return instance; }

private:
    //! Pointer to the only instance
    static T * instance;
    //! Reference counter
    static int ref;
    
    //! Flag indicating that instance was created and released before
    static bool is_second_life;

    // Friend class template
    template<class T> friend class Singleton;
};

//! Macro for easy instantiation of ThinSingleton template
#define SINGLETON(instance) ThinSingleton<instance ## Instance>

// Pointer to the only instance
template<class T> T * ThinSingleton<T>::instance;
// Reference counter
template<class T> int ThinSingleton<T>::ref;
// Flag indicating that instance was created and released before
template<class T> bool ThinSingleton<T>::is_second_life = false;

//! %Singleton pattern implementation.
/** Template for accessing the only instance of 
  * template parameter class. Automatically creates 
  * this instance on first access and destroys it 
  * when there are no more references.
  *
  * Example:
  * \code
  *   class MyInstance; // From above
  *   
  *   void test_myinstance2()
  *   {
  *       Singleton<MyInstance> my; // At this point instance of
  *                                 // MyInstance class was created
  *       // Member access:
  *       my->a();
  *       my->b = 0;
  *
  *       // When function ends MyInstance will be automatically freed
  *       // if there are no more references to it.
  *   }
  * \endcode
  */
template<class T> class Singleton
{
public:
    //! Constructor
    /** Increments the reference counter and creates
      * class instance for the first time. */
    inline Singleton() { thin_singleton.create(); }

    //! Access to instance properties and methods.
    inline T * operator ->() 
        { _ASSERT(NULL != ThinSingleton<T>::instance);
          return ThinSingleton<T>::instance; }

protected:
    //! Thin singleton as proxy object
    ThinSingleton<T> thin_singleton;
};

//! Rendering manager template
/** Rendering manager is a class for maintaining list of
  * objects that should be somehow rendered (sounds, sequences etc).
  * It allows to batch rendering by adding object to manager,
  * calling render method of added object and then flushing
  * rendering queue of manager.
  * IDObject is a class for representing managed object interface
  * with user-available declarations (to allow user change only
  * some object properties). It should have constructor from
  * ID value and conversion to ID operator.
  */
template<class T, class IDObject> class RenderManager
{
public:
    //! Constructor
    inline RenderManager() : next_id(0), queue_end(0) { queue.resize(1000); }

    //! Add object to rendering manager
    /** TDerived is a class derived from T.
      * @param[in]  obj   reference to previously created object
      *                   to be managed
      * @return  IDObject  proxy for added object to be used as identifier.
      */
    template<class TDerived>
        inline IDObject add( TDerived & obj )
            { objects[next_id] = boost::shared_ptr<T>(new TDerived(obj));
              return IDObject(next_id++); }

    //! Get T instance pointer by its id
    /** \param  id  managed object identifier */
    inline T * const get( ID id ) { return (*this)[id].get(); }

    //! Delete T instance
    /** \param  id  managed object identifier to delete */
    inline void del( ID id ) { objects.erase(id); }

    //! Flush (render) all objects
    /** Manager will flush collected batch information about
      * instances to render gathered during draw calls. No
      * actual rendering will occur before explicit flush
      * call. 
      */
    inline void flush() { clear_queue(); }
    
    //! Clear rendering queue
    inline void clear_queue() { queue_end = 0; }

    //! Adding managed object to rendering queue
    /** \param  id  managed object identifier proxy */
    void add_to_render_queue( const IDObject & id ) 
    { 
        queue[queue_end++] = id; 
    }
    
protected:
    // List of managed objects
    std::map<ID, boost::shared_ptr<T> > objects;
    typedef typename std::map<ID, typename boost::shared_ptr<T> >::iterator ObjectIter;
    // Next id number to use
    int next_id;
    
    // Get element by id
    inline boost::shared_ptr<T> operator []( ID id )
        { ObjectIter i = objects.find(id);
          _ASSERT(objects.end() != i);
          return i->second;
        }
    
    // Rendering queue
    std::vector<typename IDObject> queue;
    typedef typename std::vector<IDObject>::iterator QueueIter;
    // Queue size
    /* Queue is std::vector with preallocated size. vector::clear()
     * resizes vector and slows performance. Instead queue_end used as
     * queue boundary. */
    int queue_end;
};

#endif // _EDITOR_TEMPLATES_H_
