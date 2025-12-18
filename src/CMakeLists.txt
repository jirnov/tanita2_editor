PROJECT( engine C )

FILE( GLOB_RECURSE H_FILES *.h )
FILE( GLOB_RECURSE CPP_FILES *.cpp )

INCLUDE_DIRECTORIES(
    ${EDITOR_INCLUDE_DIR}/python
    ${EDITOR_INCLUDE_DIR}/editor
)

SET( USR_LIBRARIES
    boost
    python24
    zlib
)

SET( STD_LIBRARIES
    user32
    dbghelp
    dxguid
    dinput8
    d3d9
    d3dx9
    dxerr
    dsound
    gdi32
    shell32
    advapi32
)

USE_PRECOMPILED_HEADER( "stdafx.h" "stdafx.cpp" )

SOURCE_GROUP_EXT( ${CMAKE_CURRENT_SOURCE_DIR} *.h include )
SOURCE_GROUP_EXT( ${CMAKE_CURRENT_SOURCE_DIR} *.cpp src )

ADD_EXECUTABLE( engine WIN32 ${H_FILES} ${CPP_FILES} )
TARGET_LINK_LIBRARIES( engine ${USR_LIBRARIES} ${STD_LIBRARIES} )
INSTALL(TARGETS engine DESTINATION ${EDITOR_INSTALL_DIR})
