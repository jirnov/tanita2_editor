#pragma once
#include "Templates.h"
#include "Tanita2.h"
#include "Log.h"

//! File loading and resource handling class.
class FileManagerInstance
{
private:
    // File loader base class
    class FileLoaderBase;
    // Loads files from disk directly
    class DirectFileLoader;
    
    //! Reference to file instance
    class FileInstance
    {
    public:
        //! Destructor
        inline ~FileInstance() 
        { if (!data)
            return; 
          delete [] data; data = NULL; }
        
        //! Get file contents pointer
        inline char * const get_contents() const { return data; }
        //! Get file size
        inline int get_size() const { return size; }

    protected:/*
        //! Constructor
        inline FileInstance() : data(NULL), size(0) {}
        */
        //! Construct from file data buffer and size
        /** \param  data  pointer to file data
        * \param  size  size of file data in bytes
        * \note data pointer should not be deleted after assignment. 
        * It will be freed automatically in FileInstance destructor. */
        inline FileInstance( char * data, int size ) : data(data), size(size) {}
    
        //! File data pointer
        char * data;
        //! File size
        int size;
        
        // Friends
        friend class DirectFileLoader;
        friend class FileLoaderBase;
    };
    
public:
    //! File reference type
    typedef boost::shared_ptr<FileInstance> FileRef;

    // File contents filter base class
    class FilterBase;

    //! Constructor
    FileManagerInstance();
    //! Cleanup
    ~FileManagerInstance() {}
    
    //! Updating file manager state
    void update( DWORD dt );
    
    //! File loader hints
    enum HINT
    {
        //! No hint
        HINT_NONE = 0,
        
        //! DXTx compressed texture
        HINT_COMPRESSED_TEXTURE = 1,

        //! Disable save dds to file
        HINT_DONT_SAVE_DDS = 2,
    };

    //! Load file from disk (debug mode) or resource pack
    /** File loading function.
      * \param[in]  path  path to file
      * \return FileManagerInstance::FileRef object */
    FileRef load( PATH & path, HINT hint = HINT_NONE );

protected:
    //! Current file loader
    boost::shared_ptr<FileLoaderBase> loader;
    
    //! List of opened files
    std::map<PATH, FileRef> files;
    typedef std::map<PATH, FileRef>::iterator FileIter;
    
    // Friend class
    friend class FileLoader;
};
//! FileManager singleton definition
typedef Singleton<FileManagerInstance> FileManager;
//! Reference to file instance
typedef FileManagerInstance::FileRef FileRef;

// File manager hints redefinitions
#define FM_HINT_NONE               FileManagerInstance::HINT_NONE
#define FM_HINT_COMPRESSED_TEXTURE FileManagerInstance::HINT_COMPRESSED_TEXTURE
