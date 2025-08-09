#include "stdafx.h"
#include "FileLoader.h"
#include "FileManager.h"
#include "PrivateFileFilters.h"
#include "Config.h"

// Table of hash character integers
int PATH::hash_recode_table[256];

// Initialize hash table
void PATH::init_recode_table()
{
    for (int i = 0; i < 256; ++i)
    {
        if (i >= '0' && i <= '9')
            hash_recode_table[i] = i - '0';
        if (i >= 'a' && i <= 'f')
            hash_recode_table[i] = i - 'a' + 10;
    }
}

// Construct path from string
PATH::PATH( const char * s )
    : directory(0), filename(-1), extension(0), path_ext(false)
{
    DWORD * hash = &directory;
    // Skipping 'art\\' prefix
    s += 4;
    while (*s != '.' && *s != 0)
    {
        if (*s == '\\')
            hash = &filename;
        else
            *hash = (*hash << 4) + hash_recode_table[*s];
        s++;
    }
    if (0 == *s)
        return;
    
    s++;
    while (0 != *s)
    {
        extension = (extension << 8) + *s;
        ++s;
    }
}

// Get path string
std::string PATH::get_path_string()
{
    if (filename != -1)
        return boost::str(boost::format("art\\%x\\%d.%c%c%c") %
                   directory % filename % char(extension >> 16) % 
                   char(extension >> 8) % char(extension));
    return boost::str(boost::format("art\\%x.%c%c%c") %
               directory % char(extension >> 16) % char(extension >> 8) % 
               char(extension));
}

// Initializes resource pack loading mechanism.
FileManagerInstance::FileManagerInstance()
{ 
    loader = boost::shared_ptr<DirectFileLoader>(new DirectFileLoader());
    
    Log log;
    MEMORYSTATUS mstat;
    GlobalMemoryStatus(&mstat);
    log->print(boost::str(boost::format("Physical memory: %dMb total, %dMb available")
                   % ((ULONG)mstat.dwTotalPhys >> 20) % ((ULONG)mstat.dwAvailPhys >> 20)));
}

// Cleaning up filters and open files
FileManagerInstance::FileLoaderBase::~FileLoaderBase()
{
    for (FilterIter i = filters.begin(); filters.end() != i; ++i)
        delete *i;
}

// Constructor
FileManagerInstance::DirectFileLoader::DirectFileLoader()
{
    filters.push_back(new DdsFilter);
    filters.push_back(new PngFilter);
    filters.push_back(new WavFilter);
}

// Direct file load
FileManagerInstance::FileRef 
    FileManagerInstance::FileLoaderBase::direct_load( PATH & path)
{
#define OPEN_FILE(path)                                        \
    CreateFile(std::string(path).c_str(), GENERIC_READ,        \
               FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)

    // Getting full path
    PATH_EXT path_ext = static_cast<PATH_EXT&> (path);
    std::string filename = path_ext.get_path_string();

    // Applying filter to file name and trying to load file
    HANDLE file = INVALID_HANDLE_VALUE;
    file = OPEN_FILE(filename);
    if (INVALID_HANDLE_VALUE == file)
        throw FileNotExistException(filename);

    // Getting file size
    int size;
    size = GetFileSize(file, NULL);
    ASSERT_WINAPI_EXT(size, INVALID_FILE_SIZE);
    ASSERT(size >= 0);
    
    // Allocating memory for file contents
    char * contents = new char[size+1];
    
    // Reading file contents
    DWORD size_read;
    ASSERT_WINAPI(ReadFile(file, contents, size, &size_read, NULL));
    ASSERT(size == size_read);
    // Making it null-terminated string
    contents[size] = 0;
    
    // Applying filters
    std::string lowerpath = filename;
    std::use_facet<std::ctype<char> >(std::locale()).tolower(&lowerpath[0], &lowerpath[lowerpath.length()]);
    for (FilterIter i = filters.begin(); filters.end() != i; ++i)
        if ((*i)->match(lowerpath))
        {
            try { (*i)->apply((void **)&contents, &size, HINT_DONT_SAVE_DDS); }
            catch (Exception_ & e)
            {
                Log log;
                log->error(e.what());
                    throw FileNotExistException(filename);
            }
            break;
        }
    
    // Closing file
    CloseHandle(file);
    return FileRef(new FileInstance(contents, size));
}



// Load file from disk
FileManagerInstance::FileRef
    FileManagerInstance::DirectFileLoader::load( PATH & path, FileManagerInstance::HINT hint )
{
    // Applying filter to file name and trying to load file
    HANDLE file = INVALID_HANDLE_VALUE;
    std::string filename;
    for (FilterIter i = filters.begin(); filters.end() != i; ++i)
    {
        filename = (*i)->alter_path(path).get_path_string();
        file = OPEN_FILE(filename);
        break;
    }
    if (INVALID_HANDLE_VALUE == file)
    {
        // Opening file if it is not opened yet
        filename = path.get_path_string();
        file = OPEN_FILE(filename);
        if (INVALID_HANDLE_VALUE == file)
            throw FileNotExistException(filename);
    }

#undef OPEN_FILE
    
    // Getting file size
    int size;
    size = GetFileSize(file, NULL);
    ASSERT_WINAPI_EXT(size, INVALID_FILE_SIZE);
    ASSERT(size >= 0);
    
    // Allocating memory for file contents
    char * contents = new char[size+1];
    
    // Reading file contents
    DWORD size_read;
    ASSERT_WINAPI(ReadFile(file, contents, size, &size_read, NULL));
    ASSERT(size == size_read);
    // Making it null-terminated string
    contents[size] = 0;
    
    // Applying filters
    std::string lowerpath = filename;
    std::use_facet<std::ctype<char> >(std::locale()).tolower(&lowerpath[0], &lowerpath[lowerpath.length()]);
    for (FilterIter i = filters.begin(); filters.end() != i; ++i)
        if ((*i)->match(lowerpath))
        {
            try { (*i)->apply((void **)&contents, &size, hint); }
            catch (Exception_ & e)
            {
                Log log;
                log->error(e.what());
                    throw FileNotExistException(filename);
            }
            break;
        }
    
    // Closing file
    CloseHandle(file);
    return FileRef(new FileInstance(contents, size));
}

// File loading
FileManagerInstance::FileRef FileManagerInstance::load( PATH & path, FileManagerInstance::HINT hint )
{
    Config cfg;
    if (!cfg->get<bool>("disable_file_cache"))
    {
        // Checking if file was already loaded
        FileIter i = files.find(path);
        if (files.end() != i)
            return i->second;
    }

    ASSERT(NULL != loader.get());

    FileRef f;
    try { f = loader->load(path, hint); files[path] = f; }
    catch(FileNotExistException_ &) { f = loader->direct_load(path); }
    return f;
}

// Updating file manager state
void FileManagerInstance::update( DWORD dt )
{
    static DWORD timer;
    
    timer += dt;
    if (timer < 10000) return;
    timer = 0;
    
    MEMORYSTATUS mstat;
    GlobalMemoryStatus(&mstat);
    
    Config config;
    SIZE_T memlimit;
    try { memlimit = (UINT)config->get<int>("free_sysmem_threshold") << 20; }
    catch (...) { memlimit = 0; }
    
    if (memlimit <= 0)
        memlimit = mstat.dwTotalPhys / 7;
    
    if (mstat.dwAvailPhys > memlimit)
        return;

    ULONG meminfo = (ULONG)mstat.dwAvailPhys;
    
    // Low memory warning - freeing unused textures and opened files owned by them
    TextureManager tm;
    tm->collect(true); // running texture collector with hint to concentrate on RAM cleaning
   
    // Freeing unused file memory
    for (FileIter i = files.begin(); i != files.end();)
    {
        if (i->second.unique())
            i = files.erase(i);
        else ++i;
    }
    Log log;
    GlobalMemoryStatus(&mstat);
    if ((ULONG)mstat.dwAvailPhys < meminfo)
        return;
    meminfo = (((ULONG)mstat.dwAvailPhys - meminfo) >> 20);
    log->debug(boost::str(boost::format("Freeing unused memory: %dMb freed.") % meminfo));
}
