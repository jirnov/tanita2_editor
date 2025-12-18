#include "stdafx.h"
#include "Log.h"
#include "EditorConfig.h"
#include "resource/resource.h"

using namespace boost;

// Opens log-file and starts event logging
LogInstance::LogInstance() : log_file(NULL), show_log(false), show_log_enabled(true)
{
    log_file = fopen(LogFileName.c_str(), "wt");
    if (NULL == log_file)
    {
        MessageBox(NULL, "Unable to create log file", "Critical error",
                   MB_OK | MB_ICONERROR);
        exit(EXIT_FAILURE);
    }
    // Loading log file style sheet from resource
    std::string style_sheet;
    HRSRC resource = FindResource(NULL, MAKEINTRESOURCE(IDR_LOG_CSS), RT_HTML);
    HGLOBAL res = LoadResource(NULL, resource);
    void * ptr = NULL;
    if (NULL != res && NULL != (ptr = LockResource(res)))
    {
         int size = SizeofResource(NULL, resource);
         if (0 != size)
            style_sheet = std::string((char *)ptr, size);
    }
    // Printing log file header
    fprintf(log_file,
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01"
            " Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n"
        "<html>\n"
        "<head>\n"
        "  <title>Tanita2 engine work log</title>\n"
        "  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=cp-1251\" />\n"
        " <style type=\"text/css\">"
        "   table                 { width: 90%; margin: 20px; background-color: #eeffee; }"
        "   th, td                { padding: 5px; padding-left: 10px; padding-right: 10px; "
        "                           border-left: 1px dashed #ff0000;"
        "                           border-right: 1px dashed #ff0000;"
        "                           border-bottom: 1px solid #ffd0d0; }"
        "   th                    { text-align: left; font-size: x-large; font-family: Verdana;"
        "                           border-bottom: 1px solid #ff0000; padding-top: 10px;"
        "                           padding-bottom: 10px; background-color: #eeeeff; }"
        "   p                     { margin: 0px; }"
        "   "
        "   .info                 { }"
        "   .warning              { background-color: #ffeeee; }"
        "   .debug                { background-color: #ffffee; }"
        "   .error                { background-color: #ffeeee; }"
        "   .critical             { background-color: #ff8888; }"
        "   "
        "   .traceback            { background-color: #ccccff; }"
        "   .traceback p          { font-family: Courier New; }"
        "   "
        "   .script_traceback     { background-color: #eeeeff; }"
        "   .script_traceback p   { font-family: Courier New; }"
        "   "
        "   td.separator          { height: 50px; background-color: #ffffff;"
        "   	                    border-left:  0px none #ff0000;"
        "   	                    border-right: 0px none #ff0000;"
        "   	                    border-bottom: 1px solid #ff0000;"
        "   	                    border-top: 1px solid #ff0000; }"
        " </style>"
        "</head>\n"
        "<body>\n"
        "  <table cellspacing=\"0\" cellpadding=\"0\">\n"
        "    <tr><th>Editor events</th></tr>\n",
        style_sheet.c_str());
    //! \todo Write time stamp, OS version and hardware information
    fflush(log_file);
}

// Closes log-file
LogInstance::~LogInstance()
{
    //! \todo Write some information on exit
    fputs("  </table>\n"
          "</body>\n</html>\n", log_file);
    if (log_file) fclose(log_file);
}

// HTML-escape char sequence
std::string escape( const std::string & input )
{
    using namespace std;
    string out;
    for (string::const_iterator i = input.begin(); input.end() != i; ++i)
    {
        switch (*i)
        {
        case '<':  out += "&lt;"; break;
        case '>':  out += "&gt;"; break;
        case '&':  out += "&amp;"; break;
        case '@':  if (input.end() != i+1 && input.end() != i+2 && *(i+2) == 'P')
                   {
                       if (*(i+1) == '<')
                           out += "<pre>";
                       else if (*(i+1) == '>')
                           out += "</pre>";
                       i += 2;
                   }
                   break;
        default: out += *i;
        }
    }
    return out;
}

// Prints information string to log-file
void LogInstance::print( const std::string & prefix, const std::string & message )
{
    ASSERT(NULL != log_file);
    std::string msg_reformatted;
    const int wrapping_width = 70;
    
    // Showing log file after failures
    if ("debug" != prefix && "info" != prefix)
        // Log file opening may be disabled in config
        show_log = show_log_enabled;
    
    std::string msg = escape(message);
    std::string::size_type offset = 0;
    std::string whitespaces = "        ";
    bool preformated = false;
    bool is_traceback = false;
    
    fprintf(log_file, "    <tr><td class=\"%s\">\n", prefix.c_str());
    if ("traceback" == prefix)
    {
        fputs("      <pre>\n", log_file);
        whitespaces = "";
        preformated = true;
        is_traceback = true;
    }
    else fputs("      <p>\n", log_file);

    while (offset < msg.length())
    {
        std::string::size_type new_offset = msg.find("\n", offset);
        if (std::string::npos == new_offset)
            new_offset = msg.length();
        int count = new_offset - offset;
        
        if (!preformated && count > wrapping_width)
        {
            std::string::size_type space_offset = msg.rfind(" ", offset + wrapping_width);
            if (std::string::npos != space_offset && space_offset > offset)
            {
                count = space_offset - offset;
                new_offset = space_offset;
            }
        }
        std::string line(msg, offset, count);
        fprintf(log_file, "%s%s\n", whitespaces.c_str(), line.c_str());
        offset = new_offset + 1;
    }
    fprintf(log_file, "      %s\n    </td></tr>\n",
            is_traceback ? "</pre>" : "</p>");
    fflush(log_file);
}

// Print separator
void LogInstance::separator()
{
    static int new_id = 0;
    fprintf(log_file, "    <tr><td class=\"separator\">\n"
                      "      <a name=\"P%d\">&nbsp;</a>\n"
                      "      <script type=\"text/javascript\">\n"
                      "        window.location=\"#P%d\";\n"
                      "      </script>\n"
                      "    </td></tr>\n", new_id, new_id);
    new_id++;
}

// Constructor
Exception_::Exception_( const std::string & info,
                        const std::string & filename, int line )
{
    description = boost::str(format("Exception in file \"%1%\", line %2%. %3%")
                                    % strip_path(filename) % line % info);
}

// Strip path prefix from filename
std::string Exception_::strip_path( const std::string & filename )
{
    using namespace std;
    
    string pattern = "\\code\\tanita2\\", fname = filename;
    use_facet<ctype<char> >(locale()).tolower(&fname[0], &fname[fname.length()-1]);
    
    string::size_type offset = fname.find(pattern);
    if (string::npos == offset) return fname;
    return std::string(fname, offset + pattern.length());
}

// Constructor
WinAPIException_::WinAPIException_( const std::string & info,
                                    const std::string & filename, int line )
    : Exception_("", filename, line)
{
    DWORD error = GetLastError();
    char * message;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL, error, MAKELANGID(LANG_SYSTEM_DEFAULT, 
                  SUBLANG_DEFAULT), (char *)&message, 0, NULL);
    description += boost::str(format("\nMessage: \"%1%\" call failed with"
                                     " the following error (%2$X): %3%")
                                     % info % error % message);
    LocalFree(message);
}

// Constructor
DirectXException_::DirectXException_( const std::string & info, long hr,
                                      const std::string & filename, int line )
    : Exception_("", filename, line)
{
    description += boost::str(format("\nMessage: \"%1%\" call failed with"
                                     " the following error (%2%): %3%")
                                     % info % DXGetErrorString(hr)
                                     % DXGetErrorDescription(hr));
}

// Constructor
PythonException_::PythonException_( const std::string & filename, int line )
    : Exception_("", filename, line)
{
    try { PyErr_Print(); }
    catch (...) {}
    description = boost::str(format("Python exception. See traceback for details."));
}

// Constructor
FileNotExistException_::FileNotExistException_( const std::string & info, 
                                                const std::string & filename, int line )
    : Exception_("", filename, line)
{
    description += boost::str(format("File not found: \"%1%\"") % strip_path(info));
}
