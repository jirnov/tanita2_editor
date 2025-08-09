#ifndef _EDITOR_LOG_H_
#define _EDITOR_LOG_H_

#include "Tanita2.h"
#include "Templates.h"

//! Error message log and exception handler.
/** Directs messages to log file and console. Handles errors
  * and exceptions. */
class LogInstance
{
public:
    //! Opens log-file and starts event logging
    LogInstance();
    //! Closes log-file
    ~LogInstance();
    
//! Macro for easy printing method definition
#define method_print_(name, prefix)                 \
    inline void name( const std::string & message ) \
        { print(prefix, message); }
    
    //! Prints informational message to log file
    /** @param  message  message to print */
    method_print_(print, "info");
    //! Prints warning message to log file
    /** @param  message  warning message to print */
    method_print_(warning, "warning");
    //! Prints debug message to log file
    /** @param  message  debug message to print */
    method_print_(debug, "debug");
    //! Prints error message to log file
    /** @param  message  error message to print */
    method_print_(error, "error");
    //! Prints python traceback to log file
    /** @param  message  python traceback to print */
    method_print_(traceback, "traceback");
    
#undef method_print_

    //! Print separator
    void separator();

    //! Enable log file showing flag
    /** Set by config after reading configuration file */
    bool show_log_enabled;
    
protected:
    //! Prints message to log file
    /** @param  prefix   type of message to print
      * @param  message  message to print */
    void print( const std::string & prefix, const std::string & message );
    //! Log file handle
    FILE * log_file;
    //! Flag indication that we should open log file after program termination
    bool show_log;
};

//! Log singleton definition
typedef Singleton<LogInstance> Log;

#endif // _EDITOR_LOG_H_