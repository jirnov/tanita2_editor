#ifndef _STDAFX_H_
#define _STDAFX_H_

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// Libraries
#include <string>
#include <exception>

#include <boost/python/call.hpp>

#include <shellapi.h>
#include <mmsystem.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr9.h>
#include <dsound.h>
#define DIRECTINPUT_VERSION DIRECTINPUT_HEADER_VERSION
#include <dinput.h>
#undef DIRECTINPUT_VERSION

#include <boost/python.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>

#include <vector>
#include <map>
#include <list>

namespace bp = boost::python;

#endif // _STDAFX_H_

