#include "stdafx.h"

#ifdef ENABLE_MEMORY_LEAK_DETECTION
    #include <vld.h>
#endif

#include "Application.h"
#include "Python.h"

// Application handle
HINSTANCE hInstance;
// Application executable file name
std::string ExecutableName;

//! Display message box with exception
void display_exception( const std::string & info )
{
    MessageBox(ApplicationInstance::window,
               (std::string("Unhandled exception:\r\n") + info).c_str(), 
               "Tanita2", MB_ICONERROR | MB_OK);
}

// Main function
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    char * cmdl, int cmdShow )
{
    ::hInstance = hInstance;
    Log log;
    
    // Determining application name
    int count = 0;
    char * cmdLine = GetCommandLine();
    while (cmdLine[count] != 0 && cmdLine[count] != ' ')
        count++;
    if (count < 1)
    {
        log->error("Unable to determine application executable file name");
        return 1;
    }
    ExecutableName = std::string(cmdLine, count - 1);
    
    // Initializing hash table
    PATH::init_recode_table();

    try
    {
        // Starting application game loop
        SINGLETON(Application) app;
        
        app.create();
        app->init();
        app->run();
    }
	catch (bp::error_already_set &)
	{
		// Unhandled python exception
		python::PythonInstance::traceback();
	}
    catch (PythonException_ & e)
    {
        // Unhandled python exception
        python::PythonInstance::traceback();
        std::string info = boost::str(boost::format(
            "Unhandled exception caught in WinMain function: %1%") % e.what());
        Log log;
        log->error(info);
    }
    catch (std::exception & e)
    {
        // Unhandled exception
        std::string info = boost::str(boost::format(
            "Unhandled exception caught in WinMain function: %1%") % e.what());
        Log log;
        log->error(info);
        display_exception(info);
    }
    catch (...)
    {
        // Unknown critical exception
        std::string info = "Unknown critical exception in WinMain function.";
        log->error(info);
        display_exception(info);
    }
    return 0;
}
