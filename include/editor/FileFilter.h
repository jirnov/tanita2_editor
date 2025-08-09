#pragma once
#include "Tanita2.h"
#include "FileFilter.h"

//! File contents filter base class
/** FilterBase is base class for file loading converters/filters.
  * Upon file loading filter registered for given file type is called.
  */
class FileManagerInstance::FilterBase
{
public:
    //! Returns altered file name for file loader
    virtual PATH alter_path( const PATH & filename )
        { return filename; }
    
    //! Checks if filter should be applied to given file
    /** \param  filename  relative file path in lower case
      *
      * \return true if filter should process this file */
    virtual bool match( const std::string & filename ) = 0;
    
    // Match for Release version (optimized by speed)
    virtual bool fast_match( DWORD extension ) = 0;

    //! Apply filter to file contents
    /** This function may modify or replace original file contents 
      * with filtered data.
      * \param[in,out]  contents  pointer to file data before filtering
      * \param[in,out]  size      pointer to file size
      * \param[in]      hint      hint from file loader */
    virtual void apply( void ** contents, int * size,
                        FileManagerInstance::HINT hint ) = 0;
};
