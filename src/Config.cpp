#include "stdafx.h"
#include "Config.h"
#include "Log.h"
#include "EditorConfig.h"

// Constructor
ConfigInstance::ConfigInstance()
{
    // Configuration parameters for configure function
	database = bp::dict();
    database["config_filename"] = ConfigFileName;
    
    database["debug"] = true;
    database["editor"] = false;
    database["editor2.0"] = false;
    
    database["editor"] = true;

    // Loading configuration
	python::Python py;
	try
	{
		database = bp::call<bp::object>(bp::object(py["Lib"].attr("configure")).ptr(), database);
	}
	catch (bp::error_already_set &)
	{
		PyErr_Print();
		python::PythonInstance::traceback();
		throw;
	}
    // Enabling log file opening after crash
    Log log;
    log->show_log_enabled = get<bool>("display_log_file");
}

// Access to configuration value
template<typename T> T ConfigInstance::get( const std::string & key )
{
	TRY(return bp::extract<T>(bp::object(database[key])); );
}

// Setting configuration value
template<typename T> void ConfigInstance::set( const std::string & key, T val )
{
	TRY(database[key] = bp::object(val);)
}

// Template instantiation (skipped by doxygen)
//! \cond
template int ConfigInstance::get<int>( const std::string & );
template bool ConfigInstance::get<bool>( const std::string & );
template char * ConfigInstance::get<char *>( const std::string & );
template void ConfigInstance::set<int>( const std::string &, int );
template void ConfigInstance::set<bool>( const std::string &, bool );
template void ConfigInstance::set<std::string>( const std::string &, std::string );
//! \endcond
