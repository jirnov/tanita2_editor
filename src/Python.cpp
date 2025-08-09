#include "stdafx.h"
#include "Python.h"
#include "Log.h"
#include "Application.h"
#include "GameObject.h"
#include "Helpers.h"
#include <windows.h>
#include <d3dx9.h>

using namespace python;

// Printing D3DXVECTOR2
std::ostream & operator <<( std::ostream & out, D3DXVECTOR2 const & v )
{
    return out << boost::str(boost::format("vec2(%1%, %2%)") % v.x % v.y);
}

// Application main window handle
extern HWND toplevel_window;

namespace internals
{
// Python traceback value
std::string traceback;

//! Class for python stdout and stderr logging
class PythonLogger
{
public:
    //! Write python stdout to log
    inline void write( bp::str data ) { write_(data); }
    //! Write warning to log
    inline void write_warning( bp::str data ) { write_(data, 1); }
    //! Write debug message to log
    inline void write_debug( bp::str data ) { write_(data, 2); }
    //! Write python stderr to log
    inline void write_error( bp::str data ) { write_(data, 3); }
    //! Write python traceback to log
    inline void write_traceback( bp::str data ) { write_(data, 4); }
    //! Write separator in log file
    inline void separator() { write_(bp::str(), 5); }
    //! Flush application traceback data
    inline std::string engine_traceback()
        { std::string s(internals::traceback); if (s.size() > 0) s += '\n';
          internals::traceback.clear(); return s; }

protected:
    //! Write message to log
    /** @param  data  string value to write
      * @param  type  0 - info,  1 - warning,   2 - debug info, 
                      3 - error, 4 - traceback, 5 - separator */
    void write_( bp::str data, int type = 0 )
    {
        Log log;
        std::string msg;
        
        try { msg = bp::extract<const char *>(data); }
        catch (PythonException_ &)
        { throw Exception("Unhandled exception while writing to log from python"); }
        
        if (3 != type)
        {
            if (bp::str("\n") == data) return;
            switch (type)
            {
            case 0: log->print(msg); break;
            case 1: log->error(msg); break;
            case 2: log->debug(msg); break;
            case 4: log->traceback(msg); break;
            case 5: log->separator(); break;
            }
            return;
        }
        // Saving traceback to python traceback store
        internals::traceback += msg;
    }

    //! Bind class to python
    static void create_bindings()
    {
        using namespace bp;
        class_<PythonLogger>("_PythonLogger")
            .def("write",            &PythonLogger::write)
            .def("write_warning",    &PythonLogger::write_warning)
            .def("write_debug",      &PythonLogger::write_debug)
            .def("write_error",      &PythonLogger::write_error)
            .def("traceback",        &PythonLogger::write_traceback)
            .def("separator",        &PythonLogger::separator)
            .def("engine_traceback", &PythonLogger::engine_traceback)
            ;
    }
    friend void init_module_Tanita2();
};

// Overloads for default arguments
BOOST_PYTHON_FUNCTION_OVERLOADS(pause_overloads, ApplicationInstance::pause, 0, 1)

// ShowCursor wrapper
inline static void show_cursor( bool show ) { ShowCursor(show); }

// Edit file
inline void shell_edit( char * filename )
{
    ShellExecute(NULL, NULL, filename, NULL, NULL, SW_SHOW);
}

// Quit game
inline void quit_game()
{
    ApplicationInstance::quit_game = true;
}

// Set sounds volume
inline void set_sound_volume( int volume )
{
    ASSERT(0 <= volume && volume <= 100);
    SoundManager sm;
    sm->set_sound_volume(float(volume) / 100);
}

// Set music volume
inline void set_music_volume( int volume )
{
    ASSERT(0 <= volume && volume <= 100);
    SoundManager sm;
    sm->set_music_volume(float(volume) / 100);
}

// Save screen shot to file.
inline void save_screenshot( char * filename, int width, int height )
{
    Direct3D d3d;
    d3d->save_screenshot(filename, width, height);
}

// Module definitions for engine python interface
BOOST_PYTHON_MODULE(Tanita2)
{
    using namespace bp;

    PythonLogger::create_bindings();
    
    def("pause",  ApplicationInstance::pause, pause_overloads());
    def("resume", ApplicationInstance::resume);
    
    def("quit_game", quit_game);
    def("save_screenshot", save_screenshot);
    def("set_sound_volume", set_sound_volume);
    def("set_music_volume", set_music_volume);
    
    def("show_cursor", show_cursor);
    
    def("on_script_reload", ApplicationInstance::on_script_reload);

    def("process_messages", ApplicationInstance::process_messages);

    def("shell_edit", shell_edit);
    def("disable_autoactivation", ApplicationInstance::disable_autoactivation);

    struct vec2_pickle_suite : pickle_suite
    {
        static tuple getinitargs( const D3DXVECTOR2 & v )
        {
            return make_tuple(v.x, v.y);
        }
        
        static tuple getstate( const D3DXVECTOR2 & v )
        {
            return make_tuple(v.x, v.y);
        }
        
        static void setstate( D3DXVECTOR2 & v, tuple state )
        {
            v.x = extract<float>(state[0]);
            v.y = extract<float>(state[1]);
        }
    };

    
    // Binding D3DXVECTOR2
    class_<D3DXVECTOR2>("vec2", init<float, float>())
        .def(init<const D3DXVECTOR2 &>())
        
        .def_pickle(vec2_pickle_suite())
        
        .def(self += other<D3DXVECTOR2>())
        .def(self -= other<D3DXVECTOR2>())
        .def(self *= float())
        .def(self /= float())
        
        .def(-self)
        
        .def(self + other<D3DXVECTOR2>())
        .def(self - other<D3DXVECTOR2>())
        .def(self * float())
        .def(self / float())
        
        .def(self == other<D3DXVECTOR2>())
        .def(self != other<D3DXVECTOR2>())
        
        .def(self_ns::str(self))
       
        .def_readwrite("x", &D3DXVECTOR2::x)
        .def_readwrite("y", &D3DXVECTOR2::y)
        ;
        
    SequenceID::create_bindings();
    SoundID::create_bindings();
    tanita2_dict::create_bindings();
    ingame::GameObject::create_bindings();
}

} // End of ::internals namespace

// Initializes python interpreter
PythonInstance::PythonInstance()
{
    // Initializing python
    Py_SetProgramName("editor.exe");
    Py_Initialize();
    
    bp::object main_module = bp::object(bp::handle<>(bp::borrowed(PyImport_AddModule("__main__"))));
    main_namespace = main_module.attr("__dict__");
   
    // Importing engine exports to python
    if (PyImport_AppendInittab("Tanita2", internals::initTanita2) == -1)
        throw Exception("Unable to export engine interface to python.");

    // Setting traceback manually - we don't have error logger yet
    internals::traceback = "Exception while setting python error logger";
    std::string error_logger_setup(
        "import sys\n"
        "from Tanita2 import _PythonLogger\n"
        "sys.stdout = _PythonLogger()\n"
        "sys.stderr = _PythonLogger()\n"
        "sys.stderr.write = sys.stderr.write_error\n"
        // First trying to load from pack, then from file system
        "sys.path = ['code.pak', '.', 'Lib/std']");
    // Redirecting python output to log file
    run_string(error_logger_setup);
    // Cleaning traceback
    internals::traceback = "";
    // Initializing python script engine library
    import(std::string("Lib"));
    import("weakref");
    
    // Python callable() and getattr() functions
    callable = main_module.attr("__builtins__").attr("callable");
    getattr_func = main_module.attr("__builtins__").attr("getattr");
}

// Destructor
PythonInstance::~PythonInstance()
{
    Py_Finalize();
}

// Import python module
void PythonInstance::import( const std::string & path )
{
    bp::object module = bp::object(bp::handle<>(PyImport_Import(bp::str(path).ptr())));
    main_namespace[path] = module;
}

// Run arbitrary python code
bp::object PythonInstance::run_string( const std::string & code )
{
    return bp::object(bp::handle<>(PyRun_String(code.c_str(), Py_file_input, 
                               main_namespace.ptr(), main_namespace.ptr())));
}

// prints python traceback to user
void PythonInstance::traceback()
{
    std::string tb = internals::traceback;
    internals::traceback.clear();
    
    Log log;
    log->traceback(tb);
    ApplicationInstance::display_traceback(tb);
}
