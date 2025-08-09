#ifndef _EDITOR_TANITA2_H_
#define _EDITOR_TANITA2_H_

#include "Exceptions.h"
#include <boost/python.hpp>
//#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <windows.h>

/* Global options and definitions */

// Uncomment this to enable memory leak detection
//#define ENABLE_MEMORY_LEAK_DETECTION


// Type definition for file path
struct PATH
{
public:
    // Constructors
    inline PATH() : directory(0), filename(-1), extension(0), path_ext(false) {}
    inline PATH( const std::string & path ) { PATH::PATH(path.c_str()); }
    inline PATH( DWORD directory, DWORD filename, DWORD extension )
        : directory(directory), filename(filename), extension(extension), path_ext(false) {}
    PATH( const char * path );
    
    // Comparison
    inline bool operator ==( const PATH & p ) const
        { return p.directory == directory && p.filename == filename &&
                 p.extension == extension; }
    inline bool operator <( const PATH & p ) const
        { if (directory == p.directory) return filename < p.filename;
          else return directory < p.directory; }
    
    // Get filename and directory hash
    inline DWORD get_directory() const { return directory; }
    inline DWORD get_filename() const { return filename; }
    inline DWORD get_extension() const { return extension; }
    
    // Get path string
    std::string get_path_string();

    // Initialize hash recode table
    static void init_recode_table();
    
#define EXT(name, a, b, c) name = (((a) << 16) + ((b) << 8) + (c))

    // Commonly used extensions
    static const DWORD EXT(PNG, 'p', 'n', 'g'),
                       EXT(RGN, 'r', 'g', 'n'),
                       EXT(PTH, 'p', 't', 'h'),
                       EXT(DDS, 'd', 'd', 's'),
                       EXT(WAV, 'w', 'a', 'v'),
                       EXT(EFX, 'e', 'f', 'x');
#undef EXT
    
protected:
    // Directory and file hash, extension
    DWORD directory, filename, extension;

    // This instance can't convert to PATH_EXT
    bool path_ext;
    
    // Table of hash character integers
    static int hash_recode_table[256];

    friend struct PATH_EXT;
};

// Definition for file path with full path
struct PATH_EXT : public PATH
{
public:
    inline PATH_EXT( const std::string & path ) { PATH::PATH(path.c_str()); path_ext = true; }
    inline PATH_EXT( const char * path) : PATH(path) { this->path = path; path_ext = true; }
    inline std::string get_path_string() { if (path_ext) return path; return std::string(""); }
protected:
    std::string path;
};

#endif // _EDITOR_TANITA2_H_
