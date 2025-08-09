#ifndef _EDITOR_PRIVATEFILEFILTERS_H_
#define _EDITOR_PRIVATEFILEFILTERS_H_
#include "Tanita2.h"
#include "FileManager.h"
#include "Graphics.h"
#include "FileFilter.h"
#include "TextureManager.h"

//! PNG to DDS converter
class PngFilter: public FileManagerInstance::FilterBase
{
public:
    //! Returns true if file filter is available for filename
    virtual bool match( const std::string & filename )
    {
        if (0 == filename.compare(filename.size() - 4, 4, ".png", 0, 4))
        {
            this->filename = filename.substr(0, filename.size()-4) + ".dds";
            return true;
        }
        return false;
    }
    
    // Match for Release version (optimized by speed)
    virtual bool fast_match( DWORD extension )
    {
        throw Exception("PNG filter doesn't support data packs");
    }
    
    //! Filters file contents
    virtual void apply( void ** contents, int * size, FileManagerInstance::HINT hint )
    {
        ASSERT(NULL != contents && 0 != size);
        void * old_contents = *contents;
        
        Direct3D d3d;
        // Creating texture from contents
        LPDIRECT3DTEXTURE9 tmp_texture;
        D3DXIMAGE_INFO info;
        
        bool compress = hint & FileManagerInstance::HINT::HINT_COMPRESSED_TEXTURE;
        const D3DFORMAT compression = (D3DFORMAT)Direct3DInstance::TextureManagerInstance::DXT_METHOD;
        ASSERT_DIRECTX(D3DXCreateTextureFromFileInMemoryEx(d3d->device,
                           *contents, *size, D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, 
                           compress ? compression : D3DFMT_A8R8G8B8, 
                           D3DPOOL_SYSTEMMEM, D3DX_FILTER_NONE,
                           D3DX_FILTER_NONE, 0, &info, NULL, &tmp_texture));
        // Saving DDS texture to memory
        LPD3DXBUFFER buf;
        ASSERT_DIRECTX(D3DXSaveTextureToFileInMemory(&buf, D3DXIFF_DDS, tmp_texture, NULL));
        *size = buf->GetBufferSize();
        *contents = new char[*size];
        CopyMemory(*contents, buf->GetBufferPointer(), *size);
        // Saving original image info in reserved fields
        ((DDSURFACEDESC2 *)((char *)*contents + 4))->dwReserved = ((info.Height << 16) | info.Width);

        if (!(hint & FileManagerInstance::HINT::HINT_DONT_SAVE_DDS))
        {        
            // Saving cached texture to file
            FILE * f = fopen(filename.c_str(), "wb");
            ASSERT(NULL != f && "Saving cached texture failed");
            fwrite(*contents, *size, 1, f);
            fclose(f);
        }                
                
        // Cleanup
        delete [] old_contents;
        buf->Release();
        tmp_texture->Release();
    }
    
protected:
    std::string filename;
};

//! WAVE to simplified format converter
class WavFilter: public FileManagerInstance::FilterBase
{
public:
    //! Returns true if file filter is available for filename
    virtual bool match( const std::string & filename )
    {
        if (0 == filename.compare(filename.size() - 4, 4, ".wav", 0, 4))
            return true;
        return false;
    }
    
    virtual bool fast_match( DWORD filename )
    {
        return filename == PATH::WAV;
    }
    
    //! Checking wave file validity
    virtual void apply( void ** contents, int * size, FileManagerInstance::HINT hint )
    {
        ASSERT(NULL != contents && 0 != size);
        void * old_contents = *contents;
        
        DWORD * ptr = (DWORD *)old_contents;
        if (*ptr != 0x46464952 /* "RIFF" */)
            throw Exception("File is not a valid .wav file: missing RIFF header");
        if (*(ptr + 2) != 0x45564157 /* "WAVE" */)
            throw Exception("File is not a valid .wav file: missing WAVE signature");
        ptr += 3;
        
        if (*ptr != 0x20746d66 /* "fmt " */)
            throw Exception("File is not a valid .wav file: missing 'fmt ' block");
        if (*(WORD *)(ptr + 2) != 1 /* PCM (uncompressed) */)
            throw Exception("Compressed .wav files are not supported");
        
        // Looking for 'data' section
        int size_read = 20 + *(ptr + 1);
        BYTE * p = (BYTE *)old_contents + size_read;
        
        while (*(DWORD *)p != 0x61746164 /* "data" */)
        {
            if (size_read >= *size)
                throw Exception("File is not a valid .wav file: missing 'data' block");
            size_read += *(DWORD *)(p + 4) + 8;
            p = (BYTE *)old_contents + size_read;
        }
        // Writing offset to data chunk instead of RIFF signature
        *(DWORD *)old_contents = size_read + 4;
        if (*((DWORD *)p + 1) < 1000)
            throw Exception(".wav file is too short");
    }
};

//! DDS loader
class DdsFilter: public FileManagerInstance::FilterBase
{
public:
    //! Replaces file name from .png to .dds
    virtual PATH alter_path( const PATH & filename )
    {
        if (filename.get_extension() != PATH::PNG)
            return filename;
        
        return PATH(filename.get_directory(), filename.get_filename(), PATH::DDS);
    }

    //! Returns true if file filter is available for filename
    virtual bool match( const std::string & filename ) { return false; }
    
    virtual bool fast_match( DWORD filename ) { return false; }
    
    //! Filters file contents
    virtual void apply( void ** contents, int * size, FileManagerInstance::HINT hint )
    {
    }
};

#endif // _EDITOR_PRIVATEFILEFILTERS_H_