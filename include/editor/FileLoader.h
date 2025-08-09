#pragma once
#include "Tanita2.h"
#include "FileManager.h"

//! File loader base class
/** File loader provides file loading routines 
  * for FileManager instance. */
class FileManagerInstance::FileLoaderBase
{
public:
    //! Abstract method for file loading
    /** @param  path  path to file to load */
    virtual FileRef load( PATH & path, FileManagerInstance::HINT hint ) = 0;
    
    //! Cleaning up filters and open files
    virtual ~FileLoaderBase();
    
    //! Direct load file from direct path
    FileRef direct_load( PATH & path );

protected:
    //! File filters
    std::vector<FilterBase *> filters;
    typedef std::vector<FilterBase *>::iterator FilterIter;
};

//! Loads files directly from disk
class FileManagerInstance::DirectFileLoader:
    public FileManagerInstance::FileLoaderBase
{
public:
    //! Constructor
    DirectFileLoader();
    
    //! Load file from disk
    /** @param  path  path to file to load */
    virtual FileRef load( PATH & path, FileManagerInstance::HINT hint );
};
